//
//  MLP.cpp
//  Backpropagation
//
//  Created by Viraj Bangari on 2018-02-11.
//  Copyright © 2018 Viraj. All rights reserved.
//

#include "MLP.hpp"
#include <math.h>

MLP::MLP(
    int _numInputs,
    int _batchSize,
    int _epochs,
    float _epsilon,
    bool _verbose) :
        numInputs(_numInputs),
        batchSize(_batchSize),
        epochs(_epochs),
        // Set numOutputs to numInputs since there are no layers yet.
        numOutputs(numInputs),
        epsilon(_epsilon),
        verbose(_verbose)
{
    srand(0);
};

void MLP::addLayer(int newNumOutputs, float learningRate, float momentum)
{
    // TODO: Use heap if this is too slow
    MLPLayer newLayer(numOutputs, newNumOutputs, learningRate, momentum);
    layers.push_back(newLayer);
    numOutputs = newNumOutputs;
}

void MLP::printWeights() const {
    int i = 0;
    for (const MLPLayer& l : layers)
    {
        std::cout << "Layer: " << i << std::endl;
        std::cout << "Inputs: " << l.numberOfInputs() << " Outputs: " << l.numberOfOutputs() << '\n';
        for (const float& w : l.getWeights())
        {
            std::cout << w << ' ';
        }
        std::cout << '\n';
        i++;
    }
}

std::vector<float> MLP::predict(std::vector<float> inputs)
{
    std::vector<float>& inBetween = inputs;
    for (int i = 0; i < layers.size(); i++) {
        inBetween = layers[i].fire(inBetween);
        
        std::cout << "Output " << i << ": \n";
        for (float& f : inBetween)
        {
            std::cout << f << ' ';
        }
        std::cout << '\n';
    }
    
    return inBetween;
}

void MLP::train(
   std::vector< std::vector<float> >& X,
   std::vector< std::vector<int> >& d)
{
    if (X.size() != d.size())
    {
        std::cout << "Input vector is not the same size as the output vector\n";
        throw -1;
    }
    
    int lastTrainingIndex = (int)(0.8 * X.size());
    for (int epoch = 0; epoch < epochs; epoch++)
    {
        batchUpdate(X, d, lastTrainingIndex);
        //float error = validateModel(X, d, lastTrainingIndex + 1);
        //std::cout << "Epoch: " << epoch << ", Error: " << error << '\n';
    }
}

void
MLP::batchUpdate(std::vector< std::vector<float> >& X,
                 std::vector< std::vector<int> >& d,
                 int lastTrainingIndex)
{
    // Apply momentum based on the stored previous dWeights
    for (MLPLayer& l : layers)
    {
        l.applyMomentum();
    }
    
    int i = 0;
    while (i < lastTrainingIndex)
    {
        // Predict will update the internal previous output values
        // in each layer. This prevents needless recomputation of
        // input values for each layer during backpropagation.
        predict(X[i]);
        std::vector<int>& expected = d[i];
        
        // Adjust output layer
        // TODO: This will not work if the MLP has no hidden layers
        layers[layers.size() - 1]
            .adjustAsOutputLayer(expected,
                                 layers[layers.size() - 2]
                                     .getPreviousOutputs());
        
        // Adjust hidden nodes
        for (unsigned long j = layers.size() - 2; j > 0; j++)
        {
            const std::vector<float>& inputs =
                    layers[j - 1].getPreviousOutputs();
            layers[j]
                .adjustAsHiddenLayer(layers[j + 1], inputs);
            
        }
        
        // Adjust hidden layer
        layers[0].adjustAsHiddenLayer(layers[1], X[i]);
        
        i++;
        if (i % batchSize == 0)
        {
            for (MLPLayer& l : layers)
            {
                l.updateWeights();
            }
        }
    }
    if (i % batchSize != 0)
    {
        // Update weights, because the loop ended and they did
        // not get to properly update
        for (MLPLayer& l : layers)
        {
            l.updateWeights();
        }
    }
}

inline float
MLP::validateModel(std::vector< std::vector<float> >& X,
                   std::vector< std::vector<int> >& d,
                   int firstValidationIndex)
{
    float error = 0;
    for (int i = firstValidationIndex; i < X.size(); i++)
    {
        std::vector<float> predicted = predict(X[i]);
        std::vector<int>& expected = d[i];
        for (int j = 0; j < numOutputs; j++)
        {
            error += mse(expected[j], predicted[j]);
        }
    }
    
    return error;
}



MLPLayer::MLPLayer(int _numInputs,
                   int _numOutputs,
                   float _learningRate,
                   float _momentum) :
    numInputs(_numInputs),
    numOutputs(_numOutputs),
    learningRate(_learningRate),
    momentum(_momentum),
    weights(_numInputs * _numOutputs, 0),
    outputs(_numOutputs, 0),
    deltas(_numOutputs),
    dWeights(weights.size(), 0)
{

    for (int i = 0; i < weights.size(); i++) {
        // Set initial weights to random values between -1.0 to 1.0
        weights[i] = (rand() % 101)/100.0 - 1;
    }
    
}

void
MLPLayer::adjustAsOutputLayer(const std::vector<int>& expected,
                              const std::vector<float>& inputs)
{
    for (int i = 0; i < numOutputs; i++)
    {
        // The predicted outputs don't need to be passed in because
        // each layer keeps track of it's expected output.
        float y = outputs[i];
        float d = expected[i];
        
        // This is equivalent to (d - y) * f'(a)
        // if f is the sigmoid function.
        float delta = (d - y * y) * y * y * (1 - y * y);
        deltas[i] = delta;
        for (int j = 0; j < numInputs; j++)
        {
            int index = i * numOutputs + j;
            dWeights[index] +=
                    learningRate * deltas[i] * inputs[j];
        }
    }
}

void
MLPLayer::adjustAsHiddenLayer(const MLPLayer& nextLayer,
                              const std::vector<float>& inputs)
{
    if (numOutputs != nextLayer.numberOfInputs())
    {
        std::cout
        << "Number of outputs in next layer is not equal to number of inputs." << std::endl;
        throw -1;
    }
    
    for (int i = 0; i < outputs.size(); i++)
    {
        // Compute delta for output i
        deltas[i] = 0;
        for (int j = 0; j < outputs.size(); j++)
        {
            float y = outputs[i];
            float dlWl = nextLayer.getWeights()[j] * nextLayer.getDeltas()[j];
            deltas[i] += dlWl * y * y *  (1 - y * y);
        }
        
        for (int j = 0; j < inputs.size(); j++)
        {
            int index = i * numOutputs + j;
            dWeights[index]  += learningRate * deltas[i] * inputs[j];
        }
    }
    
    
}

void MLPLayer::applyMomentum()
{
    for (float& dw : dWeights)
    {
        dw *= momentum;
    }
}

void MLPLayer::updateWeights()
{
    for (int i = 0; i < dWeights.size(); i++)
    {
        weights[i] += dWeights[i];
    }
}

const std::vector<float>& MLPLayer::fire(const std::vector<float>& inputs)
{
    if (inputs.size() != numInputs)
    {
        throw -1;
    }
    
    for (int i = 0; i < numOutputs; i++)
    {
        float sum = 0;
        for (int j = 0; j < inputs.size(); j++)
        {
            sum += weights[i * numOutputs + j] * inputs[j];
        }
        outputs[i] = sigmoid(sum);
    }
    
    return outputs;
}

inline float sigmoid(float x)
{
    float rval =  1/(1 + exp(- 0.01 * x));
    return rval;
}

inline float dsigmoid(float x)
{
    float y = sigmoid(x);
    return y * (1 - y);
}

inline float err(float d, float y, float epsilon)
{
    if (fabs(d - y) <= epsilon)
    {
        return 0;
    }
    return d - y ;
}

inline float mse(float d, float y, float epsilon)
{
    float error = err(d, y, epsilon);
    if (error == 9)
    {
        return 0;
    }
    return pow(error, 2);
}