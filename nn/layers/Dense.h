/**
 * @file Dense.h
 *
 * @breif A Fully connected (Dense) layer
 *
 * @date 12/17/17
 * @author Ben Caine
 */

#ifndef NN_CPP_DENSE_H
#define NN_CPP_DENSE_H

#include "layers/Layer.h"


namespace nn {
    template <typename Dtype = float, int Dims = 2>
    class Dense : public Layer<Dtype, Dims> {
    public:
        /**
         * @brief Create a Dense layer
         * @param batchSize [in]: The batch size going through the network
         * @param inputDimension [in]: Expected input dimension
         * @param outputDimension [in]: The output dimensionality (number of neurons)
         * @param useBias [in]: Whether to use a bias term
         */
        explicit Dense(int batchSize, int inputDimension, int outputDimension, bool useBias);

        /**
         * @brief Return the name of the layer
         * @return The layer name
         */
        const std::string& getName() {
            const static std::string name = "Dense";
            return name;
        }

        /**
         * @brief Forward through the layer (compute the output)
         * @param input [in]: The input to the layer (either data or previous layer output)
         * @return The output of this layer
         */
        Eigen::Tensor<Dtype, Dims> forward(const Eigen::Tensor<Dtype, Dims> &input);

        /**
         * @brief Compute the gradient (backward pass) of the layer
         * @param accumulatedGrad [in]: The input to the backwards pass. (from next layer)
         * @return The output of the backwards pass (sent to previous layer)
         */
        Eigen::Tensor<Dtype, Dims> backward(const Eigen::Tensor<Dtype, Dims> &accumulatedGrad);

        /**
         * @brief Get the input shape
         * @return The input shape
         */

        Eigen::array<Eigen::Index, Dims> getOutputShape() {
            return m_outputShape;
        };


        /**
         * @brief Update weights of the layer w.r.t. gradient
         * @param learningRate [in]: Rate of change of gradient
         */
        void updateWeights(float learningRate);

        void printOutputShape() {
            std::cout << "[ ";
            for (const auto &dim : m_outputShape) {
                std::cout << dim << " ";
            }
            std::cout << "]" << std::endl;
        }

    private:
        Eigen::array<Eigen::Index, Dims> m_outputShape; ///< The output shape of this layer
        Eigen::Tensor<Dtype, Dims> m_inputCache;        ///< Cache the input to calculate gradient
        Eigen::Tensor<Dtype, Dims> m_weights;           ///< Our weights of the layer
        Eigen::Tensor<Dtype, Dims> m_bias;              ///< The bias weights if specified

        // Gradients
        Eigen::Tensor<Dtype, Dims> m_weightsGrad;       ///< The gradient of the weights
        Eigen::Tensor<Dtype, Dims> m_biasGrad;          ///< The gradient of the bias

        bool m_useBias;                                 ///< Whether we use the bias
    };

    template <typename Dtype, int Dims>
    Dense<Dtype, Dims>::Dense(int batchSize, int inputDimension, int outputDimension, bool useBias):
            m_outputShape({batchSize, outputDimension}),
            m_useBias(useBias)
    {
        m_weights = Eigen::Tensor<Dtype, Dims>(inputDimension, outputDimension);
        // TODO: Rethink how we want to init weights. Potentially use Eigen::internal::NormalRandomGenerator?
        // It should default to UniformRandomGenerator
        m_weights.setRandom();
        m_weightsGrad = Eigen::Tensor<Dtype, Dims>(inputDimension, outputDimension);
        m_weightsGrad.setZero();

        if (useBias) {
            m_bias = Eigen::Tensor<Dtype, Dims>(1, outputDimension);
            m_bias.setRandom();

            m_biasGrad = Eigen::Tensor<Dtype, Dims>(1, outputDimension);
            m_biasGrad.setZero();
        }
    };

    template <typename Dtype, int Dims>
    Eigen::Tensor<Dtype, Dims> Dense<Dtype, Dims>::forward(const Eigen::Tensor<Dtype, Dims> &input) {
        assert(input.dimensions()[1] == m_weights.dimensions()[0] &&
                            "Dense::forward dimensions of input and weights do not match");
        m_inputCache = input;

        Eigen::array<Eigen::IndexPair<int>, 1> productDims = { Eigen::IndexPair<int>(1, 0) };
        auto result = input.contract(m_weights, productDims);

        if (m_useBias) {
            // Copy the bias from (1, outputSize) to (batchDimension, outputDimension)
            return result + m_bias.broadcast(Eigen::array<Eigen::Index, 2>{m_outputShape[0], 1});
        } else {
            return result;
        }
    }

    template <typename Dtype, int Dims>
    Eigen::Tensor<Dtype, Dims> Dense<Dtype, Dims>::backward(const Eigen::Tensor<Dtype, Dims> &accumulatedGrad) {
        assert(accumulatedGrad.dimensions()[0] == m_inputCache.dimensions()[0] &&
                       "Dense::backward dimensions of accumulatedGrad and inputCache do not match");
        // m_inputCache is of shape (batchSize, inputDimension)
        // accumulatedGrad is of shape (batchSize, outputDimension)
        // So we want to contract along dimensions (0, 0), aka m_inputCache.T * accumulatedGrad
        // Where dimensions would be (inputDimension, batchSize) * (batchSize, outputDimension)
        static const Eigen::array<Eigen::IndexPair<int>, 1> transposeInput = { Eigen::IndexPair<int>(0, 0) };
        m_weightsGrad = m_inputCache.contract(accumulatedGrad, transposeInput);

        if (m_useBias) {
            m_biasGrad = accumulatedGrad.sum(Eigen::array<int, 1>{0}).eval().reshape(Eigen::array<Eigen::Index, 2>{m_outputShape[1], 1});
        }

        // accumulatedGrad is of shape (batchSize, outputDimensions)
        // m_weights is of shape (inputDimensions, outputDimensions)
        // So we want to contract along dimensions (1, 1), which would be accumulatedGrad * m_weights.T
        // Where dimensions would be (batchSize, outputDimension) * (outputDimension, inputDimension)
        static const Eigen::array<Eigen::IndexPair<int>, 1> transposeWeights = { Eigen::IndexPair<int>(1, 1)};
        return accumulatedGrad.contract(m_weights, transposeWeights);
    }

    template <typename Dtype, int Dims>
    void Dense<Dtype, Dims>::updateWeights(float learningRate) {
        m_weights += m_weightsGrad * m_weightsGrad.constant(learningRate);

        if (m_useBias) {
            m_bias += m_biasGrad * m_biasGrad.constant(learningRate);
        }
    }
}
#endif //NN_CPP_DENSE_H
