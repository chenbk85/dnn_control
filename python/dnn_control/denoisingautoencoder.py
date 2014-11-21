class DenoisingAutoencoder:
    '''
        Adapted class from http://deeplearning.net/tutorial/dA.html
        The autoencoder uses tied weights.
    '''

    def __init__(self, size_input, size_compressed, learning_rate=1.0, corruption_level=0.0, weights=None):
        from numpy import asarray, random, zeros, sqrt
        from theano import config, shared
        from theano.tensor import dmatrix
        from theano.tensor.shared_randomstreams import RandomStreams

        self.size_input = size_input
        self.size_compressed = size_compressed
        self._learning_rate = learning_rate
        self._corruption_level = corruption_level

        self._random = RandomStreams(random.randint(2 ** 30))

        if weights is None:
            initial_weights = asarray(random.uniform(
                low=-4.0 * sqrt(6.0 / (self.size_input + self.size_compressed)),
                high=4.0 * sqrt(6. / (self.size_input + self.size_compressed)),
                size=(self.size_input, self.size_compressed)), dtype=config.floatX)
            weights = shared(value=initial_weights, name='weights', borrow=True)

        bias_input = shared(value=zeros(self.size_input, dtype=config.floatX), borrow=True)

        bias_compressed = shared(value=zeros(self.size_compressed, dtype=config.floatX),
                                 name='bias_compressed', borrow=True)

        self._weights = weights

        self._bias_compressed = bias_compressed

        self._bias_input = bias_input

        self._weights_transpose = self._weights.T

        self._parameters = [self._weights, self._bias_compressed, self._bias_input]

    def _corrupt_input(self, data, corruption_level):
        from theano import config

        return self._random.binomial(size=data.shape, n=1, p=1 - corruption_level, dtype=config.floatX) * data

    def _compress(self, data):
        from theano.tensor import nnet, dot

        return nnet.sigmoid(dot(data, self._weights) + self._bias_compressed)

    def _decompress(self, compressed_data):
        from theano.tensor import nnet, dot

        return nnet.sigmoid(dot(compressed_data, self._weights_transpose) + self._bias_input)

    def _get_cost_updates(self, data):
        from theano.tensor import sum, mean, grad, log

        data_corrupted = self._corrupt_input(data, self._corruption_level)
        data_compressed = self._compress(data_corrupted)
        data_uncompressed = self._decompress(data_compressed)

        cost = mean(sum((data - data_uncompressed)**2, axis=1))
        #cost = mean(-sum(data * log(data_uncompressed) + (1 - data) * log(1 - data_uncompressed), axis=1))

        grad_parameters = grad(cost, self._parameters)
        updates = [
            (param, param - self._learning_rate * grad_param)
            for param, grad_param in zip(self._parameters, grad_parameters)
        ]

        return cost, updates

    def train(self, data, training_epochs, batch_size):
        from theano import function, shared, config
        from theano.tensor import matrix, lscalar
        from numpy import asarray, mean
        from sys import stdout

        data = shared(asarray(data, dtype=config.floatX))
        num_training_batches = data.get_value(borrow=True).shape[0] / batch_size
        data_container = matrix('x')
        index = lscalar()

        cost, updates = self._get_cost_updates(data_container)

        train_step = function([index], cost, updates=updates,
                              givens={data_container: data[index * batch_size:(index + 1) * batch_size]})

        mean_costs = []
        for epoch in xrange(training_epochs):
            costs = []
            for batch_index in xrange(num_training_batches):
                costs.append(train_step(batch_index))
                stdout.write("\r")
                stdout.write("epoch: {0} batch: {1}".format(epoch, batch_index))
                stdout.flush()

            mean_costs.append(mean(costs))

        stdout.write("\n")
        return mean_costs

    def compress(self, data):
        from theano.tensor import nnet, dot
        from theano import function

        return function([], nnet.sigmoid(dot(data, self._weights) + self._bias_compressed))()

    def decompress(self, compressed_data):
        from theano.tensor import nnet, dot
        from theano import function

        return function([], nnet.sigmoid(dot(compressed_data, self._weights_transpose) + self._bias_input))()