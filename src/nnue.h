#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <cstring>
#include <fstream>
#include "chess.hpp"

namespace NNUE
{
	enum AccumulatorOperation
	{
		Activate,
		Deactivate
	};

	const int INPUT = 768;
	const int HIDDEN = 256;
	const int OUTPUT = 1;

	const int QA = 255;
	const int QB = 64;
	const int QAB = QA * QB;

	const int CR_MIN = 0;
	const int CR_MAX = 1 * QA;
	const int SCALE = 400;

	const int ACCUMULATOR_STACK_SIZE = 512;

	using acc_t = std::array<int16_t, HIDDEN>;
	using feature_weight_t = std::array<int16_t, INPUT * HIDDEN>;
	using output_weight_t = std::array<int16_t, HIDDEN * 2 * OUTPUT>;
	using output_t = std::array<int, OUTPUT>;
	using input_t = std::array<int16_t, INPUT>;

	void SubtractFromAll(acc_t &inputA, acc_t &inputB, const feature_weight_t &delta, int offsetA, int offsetB);

	void AddToAll(acc_t &inputA, acc_t &inputB, const feature_weight_t &delta, int offsetA, int offsetB);

	void SubtractAndAddToAll(acc_t &input, const feature_weight_t &delta, int offsetS, int offsetA);

	void CReLUFlattenAndForward(acc_t &inputA, acc_t &inputB, output_weight_t &weights, output_t &output, int16_t min, int16_t max, int seperationIndex, int offset = 0);

	class BasicAccumulator
	{
	public:
		std::array<int16_t, HIDDEN> White;
		std::array<int16_t, HIDDEN> Black;
		BasicAccumulator();
		BasicAccumulator(const BasicAccumulator *acc);

		void CopyTo(const NNUE::BasicAccumulator &acc);

		void PreloadBias(const std::array<int16_t, HIDDEN> &bias);

		void Clear();
	};

	class BasicNNUE
	{
	private:
		feature_weight_t FeatureWeight;
		acc_t FeatureBias;
		output_weight_t OutWeight;
		std::array<int16_t, OUTPUT> OutBias;

		input_t WhitePov;
		input_t BlackPov;

		output_t Output;
		int CurrentAccumulator = 0;

		std::array<BasicAccumulator, ACCUMULATOR_STACK_SIZE> Accumulators;

	public:
		BasicNNUE();

		void ResetAccumulators();
		void PushAccumulator();
		void PullAccumulator();
		void RefreshAccumulator(Chess::Board &board);

		void EfficientlyUpdateAccumulator(Chess::PieceType piece, Chess::Color color, Chess::Square fromSq, Chess::Square toSq);

		template <NNUE::AccumulatorOperation Operation>
		void EfficientlyUpdateAccumulator(Chess::PieceType piece, Chess::Color color, Chess::Square sq)
		{
			const int colorStride = 64 * 6;
			const int pieceStride = 64;

			int opPieceStride = piece * pieceStride;

			int whiteIndex = color * colorStride + opPieceStride + (sq);
			int blackIndex = (~color) * colorStride + opPieceStride + (sq ^ 56);

			BasicAccumulator &accumulator = Accumulators[CurrentAccumulator];

			if (Operation == AccumulatorOperation::Activate)
			{
				WhitePov[whiteIndex] = 1;
				BlackPov[blackIndex] = 1;
				AddToAll(accumulator.White, accumulator.Black, FeatureWeight, whiteIndex * HIDDEN, blackIndex * HIDDEN);

			}
			else
			{
				WhitePov[whiteIndex] = 0;
				BlackPov[blackIndex] = 0;
				SubtractFromAll(accumulator.White, accumulator.Black, FeatureWeight, whiteIndex * HIDDEN, blackIndex * HIDDEN);
			}
		}

		int Evaluate(Chess::Color side);

		void FromJson(const std::string &file_name);
	};
};