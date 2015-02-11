from numpy import random, mean
import os
from time import clock
from sensor_data_loader import load_sensor_files
from stacked_denoising_autoencoder import StackedDenoisingAutoencoder

pretraining_epochs = 50
pretraining_learning_rate = 0.1
batch_size = 100
num_hidden_nodes = 10
corruption_level = 0.0
data_path = "/home/willist/Documents/data/"

training_set = load_sensor_files(data_path)

# compute number of minibatches for training, validation and testing
n_train_batches = training_set.get_value(borrow=True).shape[0]
n_train_batches /= batch_size

# numpy random generator
#  start-snippet-3

numpy_rng = random.RandomState(89677)
print '... building the model'
# construct the stacked denoising autoencoder class

sda = StackedDenoisingAutoencoder(
        numpy_rng=numpy_rng,
        n_ins=81,
        hidden_layers_sizes=[40, 20],
        n_outs=5)

corruption_levels = [.1, .2]

    # end-snippet-3 start-snippet-4
    #########################
    # PRETRAINING THE MODEL #
    #########################
print '... getting the pretraining functions'
pretraining_fns = sda.pretraining_functions(train_set_x=training_set,
                                                batch_size=batch_size)

print '... pre-training the model'
start_time = clock()
## Pre-train layer-wise
for i in xrange(sda.n_layers):
    # go through pretraining epochs
    for epoch in xrange(pretraining_epochs):
        # go through the training set
        c = []
        for batch_index in xrange(n_train_batches):
            c.append(pretraining_fns[i](index=batch_index,
                     corruption=corruption_levels[i],
                     lr=pretraining_learning_rate))
        print 'Pre-training layer %i, epoch %d, cost ' % (i, epoch),
        print mean(c)

end_time = clock()

print 'The pretraining code for file ' + os.path.split(__file__)[1] + ' ran for %.2fm' % ((end_time - start_time) / 60.)

# end-snippet-4
########################
# FINETUNING THE MODEL #
########################

# get the training, validation and testing function for the model
print '... getting the finetuning functions'
train_fn, validate_model, test_model = sda.build_finetune_functions(
    datasets=datasets,
    batch_size=batch_size,
    learning_rate=finetune_lr
)

print '... finetunning the model'
# early-stopping parameters
patience = 10 * n_train_batches  # look as this many examples regardless
patience_increase = 2.  # wait this much longer when a new best is
                        # found
improvement_threshold = 0.995  # a relative improvement of this much is
                               # considered significant
validation_frequency = min(n_train_batches, patience / 2)
                              # go through this many
                              # minibatche before checking the network
                              # on the validation set; in this case we
                              # check every epoch

best_validation_loss = numpy.inf
test_score = 0.
start_time = time.clock()

done_looping = False
epoch = 0

while (epoch < training_epochs) and (not done_looping):
    epoch = epoch + 1
    for minibatch_index in xrange(n_train_batches):
        minibatch_avg_cost = train_fn(minibatch_index)
        iter = (epoch - 1) * n_train_batches + minibatch_index

        if (iter + 1) % validation_frequency == 0:
            validation_losses = validate_model()
            this_validation_loss = numpy.mean(validation_losses)
            print('epoch %i, minibatch %i/%i, validation error %f %%' %
                  (epoch, minibatch_index + 1, n_train_batches,
                   this_validation_loss * 100.))

            # if we got the best validation score until now
            if this_validation_loss < best_validation_loss:

                #improve patience if loss improvement is good enough
                if (
                    this_validation_loss < best_validation_loss *
                    improvement_threshold
                ):
                    patience = max(patience, iter * patience_increase)

                # save best validation score and iteration number
                best_validation_loss = this_validation_loss
                best_iter = iter

                # test it on the test set
                test_losses = test_model()
                test_score = numpy.mean(test_losses)
                print(('     epoch %i, minibatch %i/%i, test error of '
                       'best model %f %%') %
                      (epoch, minibatch_index + 1, n_train_batches,
                       test_score * 100.))

        if patience <= iter:
            done_looping = True
            break

end_time = time.clock()
print(
    (
        'Optimization complete with best validation score of %f %%, '
        'on iteration %i, '
        'with test performance %f %%'
    )
    % (best_validation_loss * 100., best_iter + 1, test_score * 100.)
)
print >> sys.stderr, ('The training code for file ' +
                      os.path.split(__file__)[1] +
                      ' ran for %.2fm' % ((end_time - start_time) / 60.))