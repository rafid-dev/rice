#include <algorithm>
#include <cstring>
#include "nnue.h"
#include "json.hpp"
#define INCBIN_STYLE INCBIN_STYLE_CAMEL
#include "incbin/incbin.h"
#if !defined(_MSC_VER)
INCBIN(EVAL, "nn.nnue");
#else
const unsigned char gEVALData[1] = { 0x0 };
const unsigned char* const gEmbeddedNNUEEnd = &gEVALData[1];
const unsigned int gEmbeddedNNUESize = 1;
#endif

bool DEBUG = false;

// NNUE Implementation taken from StockNemo
// BasicAccumulator
NNUE::BasicAccumulator::BasicAccumulator()
{
	std::fill(std::begin(White), std::end(White), 0);
	std::fill(std::begin(Black), std::end(Black), 0);
}

void NNUE::BasicAccumulator::CopyTo(const NNUE::BasicAccumulator &acc)
{
	std::copy(std::begin(acc.White), std::end(acc.White), std::begin(White));
	std::copy(std::begin(acc.Black), std::end(acc.Black), std::begin(Black));
}

void NNUE::BasicAccumulator::PreloadBias(const std::array<int16_t, HIDDEN> &bias)
{
	std::copy(std::begin(bias), std::end(bias), std::begin(White));
	std::copy(std::begin(bias), std::end(bias), std::begin(Black));
}

void NNUE::BasicAccumulator::Clear()
{
	std::fill(std::begin(White), std::end(White), 0);
	std::fill(std::begin(Black), std::end(Black), 0);
}

// BasicNNUE

NNUE::BasicNNUE::BasicNNUE()
{
	BasicAccumulator acc;
	std::fill(Accumulators.begin(), Accumulators.end(), acc);
}

void NNUE::BasicNNUE::ResetAccumulators()
{
	CurrentAccumulator = 0;
}

void NNUE::BasicNNUE::PushAccumulator()
{
	Accumulators[CurrentAccumulator + 1].CopyTo(Accumulators[CurrentAccumulator]);
	CurrentAccumulator++;
}

void NNUE::BasicNNUE::PullAccumulator()
{
	CurrentAccumulator--;
}

void NNUE::BasicNNUE::RefreshAccumulator(Chess::Board &board)
{
	std::fill(std::begin(WhitePov), std::end(WhitePov), 0);
	std::fill(std::begin(BlackPov), std::end(BlackPov), 0);

	BasicAccumulator &accumulator = Accumulators[CurrentAccumulator];

	accumulator.Clear();
	accumulator.PreloadBias(FeatureBias);

	for (Chess::Square sq = Chess::SQ_A1; sq < Chess::NO_SQ; sq++)
	{
		Chess::PieceType p = board.pieceTypeAtB(sq);
		if (p == Chess::NONETYPE)
			continue;
		
		EfficientlyUpdateAccumulator<NNUE::AccumulatorOperation::Activate>(p, board.colorOf(sq), sq);
	}
}

void NNUE::BasicNNUE::EfficientlyUpdateAccumulator(Chess::PieceType piece, Chess::Color color, Chess::Square fromSq, Chess::Square toSq)
{
	const int colorStride = 64 * 6;
	const int pieceStride = 64;

	int opPieceStride = piece * pieceStride;

	int whiteIndexFrom = color * colorStride + opPieceStride + (fromSq);
	int blackIndexFrom = (~color) * colorStride + opPieceStride + (fromSq ^ 56);
	
	int whiteIndexTo = color * colorStride + opPieceStride + toSq;
	int blackIndexTo = (~color) * colorStride + opPieceStride + (toSq ^ 56);

	BasicAccumulator &accumulator = Accumulators[CurrentAccumulator];

	WhitePov[whiteIndexFrom] = 0;
	BlackPov[blackIndexFrom] = 0;

	WhitePov[whiteIndexTo] = 1;
	BlackPov[blackIndexTo] = 1;

	SubtractAndAddToAll(accumulator.White, FeatureWeight, whiteIndexFrom * HIDDEN, whiteIndexTo * HIDDEN);
	SubtractAndAddToAll(accumulator.Black, FeatureWeight, blackIndexFrom * HIDDEN, blackIndexTo * HIDDEN);
}

int NNUE::BasicNNUE::Evaluate(Chess::Color side)
{
	BasicAccumulator &accumulator = Accumulators[CurrentAccumulator];

	if (side == Chess::White)
	{
		CReLUFlattenAndForward(accumulator.White, accumulator.Black, OutWeight, Output, CR_MIN, CR_MAX, HIDDEN);
	}
	else
	{
		CReLUFlattenAndForward(accumulator.Black, accumulator.White, OutWeight, Output, CR_MIN, CR_MAX, HIDDEN);
	}

	return (Output[0] + OutBias[0]) * SCALE / QAB;
}

using json = nlohmann::json;

template <typename T>
void Weight(json weightRelation, T &weightsArray, const int &stride, const int k, bool flip = false)
{

	int i = 0;
	for (auto &[key, value] : weightRelation.items())
	{
		int j = 0;

		for (auto &[key2, v] : value.items())
		{
			int index = 0;
			if (flip)
				index = j * stride + i;
			else
				index = i * stride + j;

			double dvalue = static_cast<double>(v);
			weightsArray[index] = static_cast<int16_t>(dvalue * k);
			j++;
		}
		i++;
	}
}

template <typename T>
void Bias(json biasRelation, T &biasArray, int k)
{
	int i = 0;
	for (auto &[key, value] : biasRelation.items())
	{
		double dvalue = static_cast<double>(value);
		biasArray[i] = static_cast<int16_t>(dvalue * k);
		i++;
	}
}

bool NNUE::BasicNNUE::FromJson(const std::string &file_name)
{
	std::fstream stream(file_name);

	if (!stream){
		return false;
	}

	json data = json::parse(stream);

	for (auto &[key, value] : data.items())
	{
		if (key == "ft.weight")
		{
			Weight<feature_weight_t>(value, FeatureWeight, HIDDEN, QA, true);
			if (DEBUG)std::cout << "Feature weights loaded." << std::endl;
		}

		else if (key == "ft.bias")
		{
			Bias<acc_t>(value, FeatureBias, QA);
			if (DEBUG)std::cout << "Feature bias loaded." << std::endl;
		}
		else if (key == "out.weight")
		{
			Weight<output_weight_t>(value, OutWeight, HIDDEN * 2, QB);
			if (DEBUG)std::cout << "Out weights loaded." << std::endl;
		}
		else if (key == "out.bias")
		{

			Bias<std::array<int16_t, 1>>(value, OutBias, QAB);
			if (DEBUG)std::cout << "Out bias loaded." << std::endl;
		}
	}
	if (DEBUG)std::cout << "Basic NNUE loaded from JSON." << std::endl;

	return true;
}

void NNUE::BasicNNUE::ToBin(const std::string &file_name){
	std::ofstream outputData(file_name, std::ios::out | std::ios::binary);

	if (outputData){
		outputData.write((char*)(&FeatureWeight), sizeof FeatureWeight);
		outputData.write((char*)(&FeatureBias), sizeof FeatureBias);
		outputData.write((char*)(&OutWeight), sizeof OutWeight);
		outputData.write((char*)(&OutBias), sizeof OutBias);
		outputData.close();
	}
}

void NNUE::BasicNNUE::ReadBin(){
		uint64_t memoryIndex = 0;

		std::memcpy(FeatureWeight.data(), &gEVALData[memoryIndex], INPUT * HIDDEN * sizeof(int16_t));

		memoryIndex += INPUT * HIDDEN * sizeof(int16_t);

		std::memcpy(FeatureBias.data(), &gEVALData[memoryIndex],
			HIDDEN * sizeof(int16_t));
		memoryIndex += HIDDEN * sizeof(int16_t);

		std::memcpy(OutWeight.data(), &gEVALData[memoryIndex],
			HIDDEN * 2 * OUTPUT * sizeof(int16_t));

		memoryIndex += HIDDEN * 2 * OUTPUT * sizeof(int16_t);
		std::memcpy(OutBias.data(), &gEVALData[memoryIndex], 1 * sizeof(int32_t));
		memoryIndex += 1 * sizeof(int32_t);
}

void NNUE::BasicNNUE::Init(const std::string& file_name){
	if (!FromJson(file_name)){
		if(DEBUG){std::cout << "NNUE loaded from exe.\n";}
		ReadBin();
		return;
	}
	if(DEBUG){std::cout << "NNUE loaded from JSON.\n";}
}