from sensor_data_loader import load_sensor_file, load_sensor_files
import theano
import theano.tensor as T
from theano.tensor.shared_randomstreams import RandomStreams
import numpy
import time
import sys
import os
from denoising_autoencoder import DenoisingAutoencoder

learning_rate = 0.1
training_epochs = 50
batch_size = 20
num_hidden_nodes = 5
corruption_level = 0.0
data_path = "/home/willist/Documents/data/"

training_set = load_sensor_files(data_path)
# training_set = load_sensor_file("/home/willist/Documents/data/sensor_stream_03_02_15_03_54_00_gen2.txt")

print "collected " + str(training_set.get_value(borrow=True).shape[0]) + " samples"

# compute number of minibatches for training, validation and testing
n_train_batches = training_set.get_value(borrow=True).shape[0] / batch_size


# allocate symbolic variables for the data
index = T.lscalar()    # index to a [mini]batch
x = T.matrix('x')  # the data is presented as rasterized images


#####################################
# BUILDING THE MODEL CORRUPTION 30% #
#####################################

rng = numpy.random.RandomState(123)
theano_rng = RandomStreams(rng.randint(2 ** 30))

da = DenoisingAutoencoder(
    numpy_rng=rng,
    theano_rng=theano_rng,
    input=x,
    n_visible=45,
    n_hidden=num_hidden_nodes
)

cost, updates = da.get_cost_updates(
    corruption_level=corruption_level,
    learning_rate=learning_rate
)

train_da = theano.function(
    [index],
    cost,
    updates=updates,
    givens={
        x: training_set[index * batch_size: (index + 1) * batch_size]
    }
)

start_time = time.clock()

############
# TRAINING #
############

# go through training epochs
for epoch in xrange(training_epochs):
    # go through trainng set
    c = []
    for batch_index in xrange(n_train_batches):
        c.append(train_da(batch_index))

    print 'Training epoch %d, cost ' % epoch, numpy.mean(c)

end_time = time.clock()

training_time = (end_time - start_time)

print >> sys.stderr, ('The training ran for %.2fm' % (training_time / 60.))

sys.exit(0)

print da.W.get_value(borrow=True).T

samples = training_set.get_value(borrow=True)
num_test_samples = 1  # len(samples)/10
test_samples = samples[numpy.random.randint(len(samples), size=num_test_samples), :]
for sample in test_samples:
    s_compr = da.compress(sample)
    s_decompr = da.decompress(s_compr)
    print sample
    print s_compr
    print s_decompr
    print "\n"