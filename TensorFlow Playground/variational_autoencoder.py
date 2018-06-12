# Copyright 2018 - Bernhard Rieder - All Rights Reserved.
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf
import weapon_data

DEFAULT_MODEL_PATH = "./trained_vae/"

def get_untrained(session, network_architecture, optimizer, transfer_fct, batch_size=1):
    '''Convenience wrapper to get an untrained Variational Autoencoder

    Args:
        session (tensorflow.Session): Opened TensorFlow session.
        weapon_data (weapon_data.DataSet): Data obstained with weapon_data which is used to train the model.
        network_architecture (dict): The network architecture for this VAE. The second hidden layer is optional!
            E.g., network_architecture = dict(n_hidden_1=20, # 1st layer encoder/decoder neurons
                                              n_hidden_2=20, # OPTIONAL: 2nd layer encoder/decoder neurons
                                              n_input=50, # data input
                                              n_z=10)  # dimensionality of latent space
        optimizer (tensorflow.Optimizer): TensorFlow optimizer used to minimize the loss/cost function in the neural network.
            E.g., tensorflow.train.AdamOptimizer(learning_rate=0.001)
        transfer_fct: TensorFlow activation function applied after each hidden layer.
            E.g., tensorflow.tanh
        batch_size (int, optional): Size of batches to sample the latent space in the VAE.

    Returns:
        VariationalAutoencoder: A untrained VAE model.
    '''
    return VariationalAutoencoder(session=session, network_architecture=network_architecture, optimizer=optimizer,
                                 transfer_fct=transfer_fct, batch_size=batch_size, print_debug=False)

def get_new_trained(session, weapon_data, network_architecture, optimizer, transfer_fct,
                          batch_size=1, training_epochs=10, epoch_debug_step=5,
                          trained_model_save_path=DEFAULT_MODEL_PATH,
                          save_model = True, save_model_every_epoch = False, get_log_as_string=False):
    '''Convencience wrapper to get a trained Variational Autoencoderself.

    Args:
        session (tensorflow.Session): Opened TensorFlow session.
        weapon_data (weapon_data.DataSet): Data obstained with weapon_data which is used to train the model.
        network_architecture (dict): The network architecture for this VAE. The second hidden layer is optional!
            E.g., network_architecture = dict(n_hidden_1=20, # 1st layer encoder/decoder neurons
                                              n_hidden_2=20, # OPTIONAL: 2nd layer encoder/decoder neurons
                                              n_input=50, # data input
                                              n_z=10)  # dimensionality of latent space
        optimizer (tensorflow.Optimizer): TensorFlow optimizer used to minimize the loss/cost function in the neural network.
            E.g., tensorflow.train.AdamOptimizer(learning_rate=0.001)
        transfer_fct: TensorFlow activation function applied after each hidden layer.
            E.g., tensorflow.tanh
        batch_size (int, optional): Size of batches used in the training and to sample the latent space in the VAE.
        training_epochs (int, optional): Number of epochs used to train the model.
        epoch_debug_step (int, optional): Number of steps it takes to debug output the current number of epoch and its average cost in training.
        trained_model_save_path (str, optional): Path where model will be saved. Default path is: "./trained_vae/"
        save_model (bool, optional): Saves model to "trained_model_save_path" if set to True.
        save_model_every_epoch (bool, optional): Saves model after every epoch if set to True.
        get_log_as_string (bool, optional): Additionally returns the log as string instead of the output log

    Returns:
        VariationalAutoencoder: A trained VAE model.
        str: The debug log as string if get_log_as_string is set to True.
    '''
    vae = get_untrained(session=session, network_architecture=network_architecture, optimizer=optimizer,
                                 transfer_fct=transfer_fct, batch_size=batch_size)
    vae, log_str = train(vae, weapon_data, batch_size, training_epochs, epoch_debug_step,
                       trained_model_save_path, save_model, save_model_every_epoch, get_log_as_string)

    if get_log_as_string:
        return vae, log_str
    else:
        return vae

def restore(untrained_vae, model_path):
    '''Convenience wrapper to restore a model from a trained model file.

    Args:
        untrained_vae (VariationalAutoencoder): A untrained VAE.
        model_path (str): The path to a trained modelself.
            E.g., ./trained_vae/model.ckpt"
    Returns:
        VariationalAutoencoder: The trained VAE.
    '''
    untrained_vae.load_trained_model(model_path)
    return untrained_vae

def train(vae, weapon_data, batch_size=1, training_epochs=10, epoch_debug_step=5,
              trained_model_save_path=DEFAULT_MODEL_PATH,
              save_model = True, save_model_every_epoch = True,
              get_log_as_string=False):
    '''Convenience wrapper to train a Variational Autoencoder.

    Args:
        vae (VariationalAutoencoder): A untrained VAE.
        weapon_data (weapon_data.DataSet): Data obstained with weapon_data which is used to train the model.
        batch_size (int, optional): Size of batches used in the training and to sample the latent space in the VAE.
        training_epochs (int, optional): Number of epochs used to train the model.
        epoch_debug_step (int, optional): Number of steps it takes to debug output the current number of epoch and its average cost in training.
        trained_model_save_path (str, optional): Path where model will be saved. Default path is: "./trained_vae/"
        save_model (bool, optional): Saves model to "trained_model_save_path" if set to True.
        save_model_every_epoch (bool, optional): Saves model after every epoch if set to True.
        get_log_as_string (bool, optional): Additionally returns the log as string instead of the output log

    Returns:
        VariationalAutoencoder: A trained VAE model.
        str: The debug log as string if get_log_as_string is set to True.
    '''
    num_samples = weapon_data.num_examples
    trained_model_path = ""
    log_str = ""

    #training cycle
    for epoch in range(training_epochs):
        avg_cost = 0.
        total_batch = int(num_samples / batch_size)

        # Loop over all batches
        for i in range(total_batch):
            batch = weapon_data.next_batch(batch_size)

            # Fit training using batch data
            cost = vae.train_with_mini_batch(batch)

            #compute average loss/cost
            avg_cost += cost / num_samples * batch_size

        if save_model and save_model_every_epoch:
            trained_model_path = vae.save_trained_model(trained_model_save_path)

        # Display logs per epoch step
        if epoch % epoch_debug_step == 0:
            log = "Epoch:"+ '%04d' % (epoch+1) + " - Cost:" + "{:.9f}".format(avg_cost)
            if get_log_as_string:
                log_str += log + " - "
            else:
                print(log)

    if save_model and not save_model_every_epoch:
        trained_model_path = vae.save_trained_model(trained_model_save_path)

    if save_model:
        log = "Trained model saved! You can find it in '"+trained_model_path+"'"
        if get_log_as_string:
            log_str += log  + " - "
        else:
            print(log)

    return vae, log_str

#based on https://jmetzen.github.io/2015-11-27/vae.html
class VariationalAutoencoder(object):
    """ Variation Autoencoder (`VAE´_)

    Args:
        session (tensorflow.Session): Opened TensorFlow session.
        network_architecture (dict): The network architecture for this VAE. The second hidden layer is optional!
            E.g., network_architecture = dict(n_hidden_1=20, # 1st layer encoder/decoder neurons
                                              n_hidden_2=20, # OPTIONAL: 2nd layer encoder/decoder neurons
                                              n_input=50, # data input
                                              n_z=10)  # dimensionality of latent space
        optimizer (tensorflow.Optimizer): TensorFlow optimizer used to minimize the loss/cost function in the neural network.
            E.g., tensorflow.train.AdamOptimizer(learning_rate=0.001)
        transfer_fct (, optional): TensorFlow activation function applied after each hidden layer.
            E.g., tensorflow.tanh
        batch_size (int, optional): Size of batches used in the training and to sample the latent space in the VAE.
        print_debug (bool, optional): Prints debug messages of the VAE if set to True.

    .. _VAE:
        See https://arxiv.org/pdf/1606.05908v2.pdf or https://arxiv.org/pdf/1312.6114.pdf for more information
    """
    def __init__(self, session, network_architecture, optimizer, transfer_fct=tf.tanh, batch_size=1, print_debug=True):
        self._print_debug = print_debug
        self.__print("Start initializing variational autoencoder (VAE) ...")

        self._network_architecture = network_architecture
        self._transfer_fct=transfer_fct
        self._optimizer_provided = optimizer
        self._batch_size = batch_size
        self._has_2_hidden_layer = ('n_hidden_2' in network_architecture)
        self._has_2_hidden_layer = network_architecture['n_hidden_2'] > 0
        self.__print("Does VAE have 2 hidden layers? " + str(self._has_2_hidden_layer), 1)

        # Input layer
        self.X = tf.placeholder(tf.float32, shape=[None, network_architecture["n_input"]], name="input_X")

        self.__create_network()
        self.__create_loss_optimizer()

        self._session = session

        #fetch a tensorflow graph/model saver
        self._train_saver = tf.train.Saver()

        #run to initialize all tensorflow variables
        self._session.run(tf.global_variables_initializer())

        self.__print("VAE ready to use!")

    def train_with_mini_batch(self, mini_batch):
        """Train model based on mini-batch of input data.

        Args:
            mini_batch: Samples of training data - the shape need to match with the input data.

        Returns:
            float: Cost of the mini-batch.
        """
        opt, cost = self._session.run((self.optimizer, self.cost), feed_dict={self.X: mini_batch})
        return cost

    def decode_from_latent_space(self, z, matches_trained_batch_size):
        """Generate data by sampling from latent space.

        Args:
            z: Samples of data - the shape need to match with the latent space dimension.
                Furthermore, either needs to be the size of the used batch_size during
                training or just 1 dataset.
                see the 'matches_trained_batch_size' for more information
            matches_trained_batch_size (bool): Indicates if the input data has the size
                of the batch which was used during training. That is because the latent
                space was sampled with the batch size and outputs batch_size times data.

        Returns:
            [float]: if 'matches_trained_batch_size' is set to 'True' then it returns the
                reconstruction of the latent space into input space times batch_size.
                else: The reconstruction mean of the latent space input batch_size*z (z
                will be replicated batch_size times)
                see the 'matches_trained_batch_size' for more information

            The Shape of the return value is based on the given network parameters.
        """

        if matches_trained_batch_size:
            return self._session.run(self.x_reconstructed, feed_dict={self.z: z})
        else:
            samples = [z[0] for _ in range(self._batch_size)]
            return self._session.run(self.x_reconstructed_mean, feed_dict={self.z: samples})

    def encode_and_decode(self, x, matches_trained_batch_size):
        """Use VAE to reconstruct given data. Encodes and decodes in the same run.

        Args:
            x: Samples of data - the shape need to match with the latent space dimension.
                Furthermore, either needs to be the size of the used batch_size during
                training or just 1 dataset.
                see the 'matches_trained_batch_size' for more information
            matches_trained_batch_size (bool): Indicates if the input data has the size
                of the batch which was used during training. That is because the latent
                space was sampled with the batch size and outputs batch_size times data.

        Returns:
            [float]: if 'matches_trained_batch_size' is set to 'True' then it returns the
                reconstruction of the input times batch size.
                else: The reconstruction mean of the input times batch_size (the input x
                will be replicated batch_size times)
                see the 'matches_trained_batch_size' for more information

            The Shape of the return value is based on the given network parameters.
        """
        if matches_trained_batch_size:
            return self._session.run(self.x_reconstructed, feed_dict={self.X: x})
        else:
            samples = [x[0] for _ in range(self._batch_size)]
            return self._session.run(self.x_reconstructed_mean, feed_dict={self.X: samples})

    def load_trained_model(self, save_path):
        """Loads trained model from disk.
        CAUTION: Needs an open session with 'tf.Session(graph=tf.Graph())'!

        Args:
            save_path (str): Path to the trained model file.
        """
        self._train_saver.restore(self._session, save_path)
        self.__print("Trained model found in '"+save_path+"' restored!")

    def save_trained_model(self, path):
        """Saves the current trained model.
            (This saves all caluclated weights)

        Args:
            path (str): Path to where the model will be saved in.

        Returns:
            str: The full of the save model file.
        """
        save_path = self._train_saver.save(self._session, path + "model.ckpt")
        self.__print("Model saved in file: {}".format(save_path))
        return save_path


    def calculate_z(self, X):
        """Calculates the sampled latent space z for a given dataset X.

        Args:
            x: Batch samples of data - the shape need to match with the input data.
                Batch size needs to be the same as used during training.

        Returns:
            [float]: The sampled latent space z (+ Gaussian distribution)

            The Shape of the return value is based on the given network parameters.
        """
        return self._session.run(self.z, feed_dict={self.X: X})

    def calculate_z_mean(self, X):
        """Calculates the mean of the latent space z with a given dataset X.

        Args:
            X: Samples of data - the shape need to match with the input data.

        Returns:
            [float]: The mean of the latent space z.

            The Shape of the return value is based on the given network parameters.
        """
        return self._session.run(self.z_mean, feed_dict={self.X: X})

    def calculate_loss(self, batch):
        """Calculates the loss for a given batch of samples

        Args:
            batch: Batch samples of data - the shape need to match with the input data.
                Batch size needs to be the same as used during training.

        Returns:
            float: Cost of the mini-batch.
        """
        cost = self._session.run(self.cost, feed_dict={self.X: batch})
        return cost

    #debug helper function to print debug messages
    def __print(self, message, indent=0):
        if self._print_debug:
            tabs=""
            for i in range(indent):
                tabs += "\t"
            print(tabs+message)

    def __create_network(self):
        self.__print("Start creating VAE network ...", 1)

        #init all weights and biases used in the network
        weights_and_biases = self.__init_weights_and_biases()

        #create the encoder network which generates the latent network
        self.z_mean, self.z_log_sigma_sq = \
            self.__create_encoder_network(weights_and_biases['weights_encoder'],
                                          weights_and_biases['biases_encoder'])

        #create the sampling operation to:
        # -) map our data distribution to a gaussian normal distribution
        # -) and secure a working backpropagation
        self.z = self.__create_z_sampling_operation()

        #create the decoder network which generates a reconstruction of input X
        self.x_reconstructed = \
            self.__create_decoder_network(weights_and_biases['weights_decoder'],
                                          weights_and_biases['biases_decoder'])

        self.x_reconstructed_mean = tf.reduce_mean(self.x_reconstructed, axis=0)

        self.__print("Finished creating VAE network!", 1)


    def __create_encoder_network(self, weights, biases):
        self.__print("Start creating encoder network ...", 2)

        #create the hidden layer based on the network params
        hidden_layer = self.__create_hidden_layer(self.X, weights, biases)

        #parameters to map the gaussian distribution to our data distribution
        z_mean = tf.matmul(hidden_layer, weights['z_mean']) + biases['z_mean']
        z_log_sigma_sq = tf.matmul(hidden_layer, weights['z_ls2']) + biases['z_ls2']

        self.__print("Finished creating encoder network!", 2)

        return (z_mean, z_log_sigma_sq)

    def __create_z_sampling_operation(self):
        self.__print("Start creating z (latent layer) sampling operation ...", 2)

        #get the latent space dimension
        n_z = self._network_architecture['n_z']

        #sample a random epsilon from a gaussian normal distribution to approximate the gaussian
        epsilon = tf.random_normal((self._batch_size, n_z), 0, 1, dtype=tf.float32)

        # sample z from a normal (gaussian) distribution -> z = mu + sigma*epsilon
        z = self.z_mean + tf.multiply(tf.sqrt(tf.exp(self.z_log_sigma_sq)), epsilon)

        self.__print("Finished creating z sampling operation!", 2)

        return z

    def __create_decoder_network(self, weights, biases):
        self.__print("Start creating decoder/generator network ...", 2)

        #create the hidden layer based on the network params
        hidden_layer = self.__create_hidden_layer(self.z, weights, biases)

        #create the operation which finally recreates the input X
        x_reconstructed = tf.matmul(hidden_layer,  weights['out']) + biases['out']

        self.__print("Finished creating decoder/generator network!", 2)

        return x_reconstructed

    def __create_hidden_layer(self, x, weights, biases):
        self.__print("Start creating hidden layer ...", 3)

        # First hidden layer
        hidden_layer_1 = tf.matmul(x, weights['h1']) + biases['h1']
        hidden_layer_1 = self._transfer_fct(hidden_layer_1)

        # Second hidden layer
        hidden_layer_2 = tf.matmul(hidden_layer_1, weights['h2']) + biases['h2']
        hidden_layer_2 = self._transfer_fct(hidden_layer_2)

        self.__print("Finished creating hidden layer!", 3)

        return hidden_layer_2 if self._has_2_hidden_layer else hidden_layer_1

    def __init_weights_and_biases(self):
        self.__print("Start initalizing weights and biases ...", 2)

        n_input = self._network_architecture['n_input']
        n_hidden_1 = self._network_architecture['n_hidden_1']
        n_hidden_2 = self._network_architecture['n_hidden_2'] if self._has_2_hidden_layer else 1
        n_hidden_out = n_hidden_2 if self._has_2_hidden_layer else n_hidden_1
        n_z = self._network_architecture['n_z']

        weights_and_biases = dict()

        #encoder
        weights_and_biases['weights_encoder'] = {
            'h1': self.__create_weight([n_input, n_hidden_1], "w_enc_h1"),
            'h2': self.__create_weight([n_hidden_1, n_hidden_2], "w_enc_h2"),
            'z_mean': self.__create_weight([n_hidden_out, n_z], "w_z_mean"),
            'z_ls2': self.__create_weight([n_hidden_out, n_z], "w_z_ls2")
        }
        weights_and_biases['biases_encoder'] = {
            'h1': self.__create_bias([n_hidden_1], "b_enc_h1"),
            'h2': self.__create_bias([n_hidden_2], "b_enc_h2"),
            'z_mean': self.__create_bias([n_z], "b_z_mean"),
            'z_ls2': self.__create_bias([n_z], "b_z_ls2")
        }

        #decoder
        weights_and_biases['weights_decoder'] = {
            'h1': self.__create_weight([n_z, n_hidden_1], "w_dec_h1"),
            'h2': self.__create_weight([n_hidden_1, n_hidden_2], "w_dec_h2"),
            'out': self.__create_weight([n_hidden_out, n_input], "w_out")
        }
        weights_and_biases['biases_decoder'] = {
            'h1': self.__create_bias([n_hidden_1], "b_dec_h1"),
            'h2': self.__create_bias([n_hidden_2], "b_dec_h2"),
            'out': self.__create_bias([n_input], "b_out")
        }
        self.__print("Finished initalizing weights and biases!", 2)

        return weights_and_biases

    def __create_weight(self, shape, name=""):
        initial = tf.contrib.layers.xavier_initializer()
        return tf.Variable(initial(shape), dtype=tf.float32, name=name)

    def __create_bias(self, shape, name=""):
        initial = tf.contrib.layers.xavier_initializer()
        return tf.Variable(initial(shape), dtype=tf.float32, name=name)

    def __create_loss_optimizer(self):
        self.__print("Start creating optimizer/backprop operation ...", 1)

        reconstr_loss = self.__l2_loss(self.x_reconstructed, self.X)
        latent_loss = self.__kullback_leibler(self.z_mean, tf.log(tf.sqrt(tf.exp(self.z_log_sigma_sq))))

        self.cost = tf.reduce_mean((reconstr_loss + latent_loss), name="cost")
        self.optimizer = self._optimizer_provided.minimize(self.cost)

        self.__print("Finished creating optimizer/backprop operation!", 1)

    #more information about why using L2 loss/MSE -> http://aoliver.org/why-mse
    def __l2_loss(self, obs, actual):
        return tf.reduce_sum(tf.square(obs - actual), 1)

    #from https://github.com/RuiShu/micro-projects/tree/master/tf-vae
    def __kullback_leibler(self, mu, log_sigma):
        # = -0.5 * (1 + log(sigma**2) - mu**2 - sigma**2)
        return -0.5 * tf.reduce_sum(1 + 2 * log_sigma - mu**2 - tf.exp(2 * log_sigma), 1)
