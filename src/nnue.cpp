#include <algorithm>
#include "nnue.h"
#include "json.hpp"

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
	BlackPov[whiteIndexFrom] = 0;
	WhitePov[whiteIndexTo] = 1;
	BlackPov[blackIndexTo] = 1;

	SubtractAndAddToAll(accumulator.White, FeatureWeight, whiteIndexFrom * HIDDEN, whiteIndexTo * HIDDEN);
	SubtractAndAddToAll(accumulator.White, FeatureWeight, blackIndexFrom * HIDDEN, blackIndexTo * HIDDEN);
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

void NNUE::SubtractFromAll(acc_t &inputA, acc_t &inputB, const feature_weight_t &delta, int offsetA, int offsetB)
{
	for (int i = 0; i < HIDDEN; i++)
	{
		inputA[i] -= delta[offsetA + i];
		inputB[i] -= delta[offsetB + i];
	}
}

void NNUE::AddToAll(acc_t &inputA, acc_t &inputB, const feature_weight_t &delta, int offsetA, int offsetB)
{
	for (int i = 0; i < HIDDEN; i++)
	{
		inputA[i] += delta[offsetA + i];
		inputB[i] += delta[offsetB + i];
	}
}

void NNUE::SubtractAndAddToAll(acc_t &input, const feature_weight_t &delta, int offsetS, int offsetA)
{
	for (int i = 0; i < HIDDEN; i++)
	{
		input[i] = input[i] - delta[offsetS + i] + delta[offsetA + i];
	}
}

void NNUE::CReLUFlattenAndForward(acc_t &inputA, acc_t &inputB, output_weight_t &weights, output_t &output, int16_t min, int16_t max, int seperationIndex, int offset)
{
	int inputSize = HIDDEN * 2;
	int weightStride = 0;

	auto RelativeIndex = [&](int index)
	{ return index < seperationIndex ? index : index - seperationIndex; };
	auto RelativeInput = [&](int index)
	{ return index < seperationIndex ? inputA : inputB; };

	for (int i = 0; i < OUTPUT; i++)
	{
		int sum = 0;
		for (int j = 0; j < inputSize; j++)
		{
			int16_t input = RelativeInput(j)[RelativeIndex(j)];
			int16_t weight = weights[weightStride + j];
			sum += std::max(min, std::min(input, max)) * weight;
		}
		output[offset + i] = sum;
		weightStride += inputSize;
	}
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

void NNUE::BasicNNUE::FromJson(const std::string &file_name)
{
	std::fstream stream(file_name);

	json data = json::parse(stream);

	for (auto &[key, value] : data.items())
	{
		if (key == "ft.weight")
		{
			Weight<feature_weight_t>(value, FeatureWeight, HIDDEN, QA, true);
			std::cout << "Feature weights loaded." << std::endl;
		}

		else if (key == "ft.bias")
		{
			Bias<acc_t>(value, FeatureBias, QA);
			std::cout << "Feature bias loaded." << std::endl;
		}
		else if (key == "out.weight")
		{
			Weight<output_weight_t>(value, OutWeight, HIDDEN * 2, QB);
			std::cout << "Out weights loaded." << std::endl;
		}
		else if (key == "out.bias")
		{

			Bias<std::array<int16_t, 1>>(value, OutBias, QAB);
			std::cout << "Out bias loaded." << std::endl;
		}
	}
	std::cout << "Basic NNUE loaded from JSON." << std::endl;
}