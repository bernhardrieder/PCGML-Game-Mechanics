from __future__ import absolute_import
from __future__ import division

import unreal_engine as ue
import numpy as np
import tensorflow as tf
import variational_autoencoder as vae
from training_data import weapon_data as weapons

from tensorflow.python.framework import random_seed

class WeaponGeneratorAPI(TFPluginAPI):
    def onSetup(self):
        self._random_seed = 19071991
        seed, _ = random_seed.get_seed(self._random_seed)
        np.random.seed(seed)

        self.shouldStop = False

        self._train_data, self._test_data = weapons.get_data(2, 14, 0, seed=self._random_seed)

        #set training parameter
        self._network_architecture = \
            dict(n_input=self._train_data.num_features,
                 n_hidden_1=26,
                 n_hidden_2=12,
                 n_z=2)
        self._batch_size = 4
        self._learning_rate = 0.01
        self._optimizer = tf.train.RMSPropOptimizer(self._learning_rate)
        self._transfer_fct = tf.nn.elu
        self._num_training_epochs = 70

        #keep track of the received and dismantled weapons
        self._dismantled_weapons = []

        #amount of dismantles models needed to retrain the model
        self._dismantled_weapons_needed_to_retrain = 20

        #start session is realized in on begin training and will be swaped with the tmp afterwards
        self._sess = None
        self._vae = None

        self._trained_model_available = False
        self._trained_model_path = ""

    def onJsonInput(self, jsonInput):
        if self._trained_model_available:
            self.__load_trained_model()

        if self._vae == None:
            ue.log("ERROR: there is no trained model?!")
            return { 'success' : 'false'}

        if not bool(jsonInput):
            ue.log("ERROR: empty input!")
            return { 'success' : 'false'}

        #clear the input data
        del jsonInput['success']

        generated_weapon = []

        #encode the json input to a standardized weapon data
        encoded_json_input = self.__encode_json_input_to_a_standardized_train_data_format(jsonInput)
        #check if the encoded data should be used to generate a new one
        #calculate loss requires a batch which has the size of the trained model batch_size
        batch = [encoded_json_input[0] for _ in range(self._batch_size)]
        generation_cost = self._vae.calculate_loss(batch)
        generation_cost /= self._batch_size

        #if the cost is too high, then just generate a random one
        #a too high value means that the VAE don't know which weapon that should be!
        if generation_cost >= 50 or np.isnan(generation_cost) or np.isinf(generation_cost):
            generated_weapon = self.__generate_random_weapons(1)
            ue.log("Generated a random weapon!")
        else:
            generated_weapon = self._vae.encode_and_decode(encoded_json_input, False)
            ue.log("Generated a new weapon based on a dismantled one!")

        if len(generated_weapon) <= 0:
            ue.log("ERROR: no generated weapon?!")
            return {'success' : 'false'}

        #do it afterwards so that crazy weapons don't destroy the model
        self.__add_received_dismantled_weapon(encoded_json_input[0])

        result, _ = self._train_data.decode_processed_tensor(generated_weapon[0])
        result['success'] = 'true'
        return result

    def onBeginTraining(self):
        with tf.Session() as sess:

            #add the dismantled weapons so that the model emerges in a direction
            if len(self._dismantled_weapons) > 0:
                self._train_data.add_new_weapons_and_restandardize_data(self._dismantled_weapons)
                self._dismantled_weapons = []
                self._num_training_epochs += 10

            network = vae.get_untrained(sess, self._network_architecture, self._optimizer,
                                      self._transfer_fct, self._batch_size)

            num_samples = self._train_data.num_examples
            ue.log("Num of training samples = %i" %num_samples)

            #is basically the same code from the VAE file
            #training cycle
            for epoch in range(self._num_training_epochs):
                avg_cost = 0.
                total_batch = int(num_samples / self._batch_size)

                # Loop over all batches
                for _ in range(total_batch):
                    batch = self._train_data.next_batch(self._batch_size)

                    # Fit training using batch data
                    cost = network.train_with_mini_batch(batch)

                    #compute average loss/cost
                    avg_cost += cost / num_samples * self._batch_size

                # Display logs per epoch step
                if epoch % 10 == 0:
                    ue.log("Epoch:"+ '%04d' % (epoch) + " - Cost:" + "{:.2f}".format(avg_cost))

                if self.shouldStop:
                    break;

            self._trained_model_path = network.save_trained_model("./vae_model/")
            self._trained_model_available = True


    def onStopTraining(self):
        if self._sess:
            self._sess.close()

    def __load_trained_model(self):
        if self._sess:
            self._sess.close()
        tf.reset_default_graph()

        self._sess = tf.Session(graph=tf.get_default_graph())
        self._vae = vae.get_untrained(self._sess, self._network_architecture, self._optimizer,
                                  self._transfer_fct, self._batch_size)
        self._vae = vae.restore(self._vae, self._trained_model_path)
        self._trained_model_available = False

    def __encode_json_input_to_a_standardized_train_data_format(self, json_input):
        prepared_for_encoding = self._train_data.prepare_decoded_tensor_dict_for_encoding(json_input)
        encoded, _ = self._train_data.encode_features_dict(prepared_for_encoding)
        encoded_standardized = self._train_data.standardize_encoded_data(encoded[0])
        return [encoded_standardized]

    def __add_received_dismantled_weapon(self, weapon):
        self._dismantled_weapons.append(weapon)

        if len(self._dismantled_weapons) >= self._dismantled_weapons_needed_to_retrain:
            self.shouldRetrain = True
            ue.log("Should retrain!")

    def __generate_random_weapons(self, num):
        generated_weapons = []

        for _ in range(num):
            random_val = np.random.normal(size=(1, self._network_architecture["n_z"]))
            weapons = self._vae.decode_from_latent_space(random_val, False)
            [generated_weapons.append(weapon) for weapon in weapons]

        return generated_weapons

#NOTE: this is a module function, not a class function. Change your CLASSNAME to reflect your class
#required function to get our api
def getApi():
	#return CLASSNAME.getInstance()
	return WeaponGeneratorAPI.getInstance()
