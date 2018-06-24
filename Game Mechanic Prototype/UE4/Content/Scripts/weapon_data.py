# Copyright 2018 - Bernhard Rieder - All Rights Reserved.
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf
import csv
import operator

from tensorflow.python.framework import random_seed

DEFAULT_TRAINING_DATA = "training_data.csv"
DEFAULT_TEST_DATA = "test_data.csv"

#define constants for data processing, not listed params are omitted from the input file
CATEGORICAL_PARAMS  = ['type', 'firemode']
NUMERICAL_PARAMS = ['damages_first', 'damages_last', 'distances_last', 'rof', 'magsize', 'reloadempty', 'shotspershell', 'hiprecoilright',
                    'hiprecoilup', 'distances_first', 'initialspeed', 'hiprecoildec', 'hipstandbasespreaddec', 'hipstandbasespreadinc' ]

#additionally define the cagegories for the encoding nad decoding
WEAPON_TYPES = ['Shotgun', 'Pistol', 'Rifle', 'SMG', 'Sniper', 'MG']
WEAPON_FIREMODES = ['Automatic', 'Semi', 'Single']
#create dict so we can iterate them
CATEGORICAL_PARAMS_DEFINES_DICT = {'type':WEAPON_TYPES, 'firemode':WEAPON_FIREMODES}


#quick'n'dirty convenience wrapper
def get_data(training_data_source=DEFAULT_TRAINING_DATA, test_data_source=DEFAULT_TEST_DATA, seed=19071991, debug=False):

    training_data = DataSet(data_source=training_data_source, seed=seed, show_debug=debug)
    test_data = DataSet(data_source=test_data_source, seed=seed, show_debug=debug)

    return training_data, test_data

def debug_printDict(dictionary):
    for key, value in dictionary.items():
        print(key, "=", value)

#based on https://github.com/tensorflow/tensorflow/blob/r1.8/tensorflow/contrib/learn/python/learn/datasets/mnist.py
class DataSet:
    def __init__(self,
                 data_source,
                 seed=None,
                 show_debug=False):

        self._show_debug = show_debug
        seed1, seed2 = random_seed.get_seed(seed)
        np.random.seed(seed1 if seed is None else seed2)

        #set the parameters
        self._categorical_params = CATEGORICAL_PARAMS
        self._numerical_params = NUMERICAL_PARAMS

        #read the data source and extract all the defined features
        features = self.__getCsvAsDict(data_source)

        #now encode and standardize those features
        self._data, self._feature_cols_to_vars_dict = self.__encodeFeatures(features)

        self._num_examples, self._num_features = self._data.shape

        if self._show_debug:
            print("Found %i features with %i values in data source %s! \n" %(self._num_features, self._num_examples, data_source))

        self._epochs_completed = 0
        self._index_in_epoch = 0

    @property
    def data(self):
        return self._data

    @property
    def num_features(self):
        return self._num_features

    @property
    def num_examples(self):
        return self._num_examples

    @property
    def standardized_max_values(self):
        return self._standardized_max_values

    @property
    def standardized_min_values(self):
        return self._standardized_min_values

    def __shuffleData(self):
        perm = np.arange(self._num_examples)
        np.random.shuffle(perm)
        self._data = self.data[perm]

    def next_batch(self, batch_size, shuffle=True):
        '''Return the next 'batch_size' examples from this data set.'''

        start = self._index_in_epoch;

        # Shuffle for the first epoch
        if self._epochs_completed == 0 and start == 0 and shuffle:
            self.__shuffleData()

        # Go to the next epoch
        if start + batch_size > self._num_examples:
            # Finished epoch
            self._epochs_completed += 1
            if self._show_debug:
                print("WeaponDataSet completed an epoch! Epoch count = %i" %self._epochs_completed )

            # Get the rest examples in this epoch
            rest_num_examples = self._num_examples - start
            data_rest_part = self._data[start:self._num_examples]

            # Shuffle the data
            if shuffle:
                self.__shuffleData()

            # Start next epoch
            start = 0
            self._index_in_epoch = batch_size - rest_num_examples
            if self._show_debug:
                print("Current position in dataset after next_batch = %i" %self._index_in_epoch)
            end = self._index_in_epoch
            data_new_part = self._data[start:end]

            return np.concatenate((data_rest_part, data_new_part), axis = 0)

        else:
            self._index_in_epoch += batch_size
            if self._show_debug:
                print("Current position in dataset after next_batch = %i" %self._index_in_epoch)
            end = self._index_in_epoch
            return self._data[start:end]

    def decode_processed_tensor(self, tensor):
        '''Decodes a processed tensor and returns a dict and the unstandardized tensor'''

        unstandardized_tensor = self.__un_standardize_columns(tensor, self._data_original)

        idx =  0 #keep track of the id to find the right value in the processed tensor
        result = dict()
        for key in self._feature_cols_to_vars_dict:
            '''
            e.g., key = _NumericColumn(key='bdrop', shape=(1,), default_value=None,
                        dtype=tf.float32, normalizer_fn=None)
            then key[0] = 'bdrop'
            '''
            if key[0] in self._numerical_params:
                result[key[0]] = str(unstandardized_tensor[idx])
                idx += 1
                '''
                e.g., key = _IndicatorColumn(categorical_column=_VocabularyListCategoricalColumn(
                            key='firemode', vocabulary_list=('Automatic', 'Semi-Automatic',
                            'Single-Action'), dtype=tf.string, default_value=-1, num_oov_buckets=0))
                then key[0] = _VocabularyListCategoricalColumn(
                            key='firemode', vocabulary_list=('Automatic', 'Semi-Automatic',
                            'Single-Action'), dtype=tf.string, default_value=-1, num_oov_buckets=0)
                and key[0][0] = 'firemode'
                and key[0][0][0] = 'Automatic'
                '''
            elif (len(key[0]) > 0) and key[0][0] in self._categorical_params:
                for i in range(0, len(key[0][1])):
                    param = key[0][0] + "_" + key[0][1][i]
                    result[param] = str(unstandardized_tensor[idx])
                    idx += 1

        return result, unstandardized_tensor

    def add_new_weapons_and_restandardize_data(self, processed_tensors):
        unstandardized = [self.__un_standardize_columns(tensor, self._data_original) for tensor in processed_tensors]
        self._data_original = np.append(self._data_original, unstandardized, axis=0)
        self._data = self.__standardize_columns(self._data_original)

        self._num_examples = self._data.shape[0]
        if self._show_debug:
            print("Added new weapon/s! New number of examples in dataset = %i" %self._num_examples)

    def encode_features_dict(self, features):
        '''Encodes a features dict which represents the training data or any other dataset
            and returns the encoded data and a dictionary which columns belongs to which value'''
        #convert all numerical data to float so they can be used as 'tf.feature_column.numeric_column'
        for key, values in features.items():
            if key in self._numerical_params:
                features[key] = [float(value) for value in values]

        #create list where all feature columns come in
        columns = [tf.feature_column.numeric_column(param) for param in self._numerical_params]

        for category in self._categorical_params:
            #one hot encoding of categorical data
            cat_column = tf.feature_column.categorical_column_with_vocabulary_list(key=category, vocabulary_list=CATEGORICAL_PARAMS_DEFINES_DICT[category])
            cat_column = tf.feature_column.indicator_column(cat_column) #could use embedding column to further reduce dimensions
            columns.append(cat_column)

        #make sure you know whats going on inside
        cols_to_vars_dict = {}

        #create an input layer with your feature columns and make sure you get the cols_to_vars dictionary!
        inputs = tf.feature_column.input_layer(features, columns, cols_to_vars=cols_to_vars_dict, trainable = False)

        if self._show_debug:
            print("Feature columns to tensor variables dictionary:")
            debug_printDict(cols_to_vars_dict)
            print()

            print("Generated input with feature columns:")
            print(inputs)
            print()

        var_init = tf.global_variables_initializer()
        table_init = tf.tables_initializer() #needed for the feature columns

        weapon_data = [];

        with tf.Session() as sess:
            sess.run((var_init, table_init))
            weapon_data = sess.run(inputs)

        original_data = np.array(weapon_data)
        return original_data, cols_to_vars_dict

    def standardize_encoded_data(self, data):
        '''Standardizes the data based on the mean and standard deviation of the original data of this class'''
        std = self._data_original.std(dtype=np.float64, axis=0)
        mean = self._data_original.mean(dtype=np.float64, axis=0)
        data_standardized = (data - mean) / std
        return data_standardized

    def prepare_decoded_tensor_dict_for_encoding(self, decoded_tensor_dict):
        '''Cleans up the encoded features in the dictionary'''

        key_type = "type"
        key_firemode = "firemode"
        #save all the values of the types and firemodes in a dict
        type_values = {}
        firemode_values = {}

        #the final dict
        prepared_for_encoding = {}

        for key,value in decoded_tensor_dict.items():
            if key_type in key:
                type_values[key] = value
            elif key_firemode in key:
                firemode_values[key] = value
            else:
                prepared_for_encoding[key] = [value]

        #now check which of them has the highest value and use this as the definite type and firemode
        type_with_max_value = max(type_values.items(), key=operator.itemgetter(1))[0]
        prepared_for_encoding[key_type] = [type_with_max_value.replace(key_type, "")]

        firemode_with_max_value = max(firemode_values.items(), key=operator.itemgetter(1))[0]
        prepared_for_encoding[key_firemode] = [firemode_with_max_value.replace(key_firemode, "")]

        return prepared_for_encoding

    def __encodeFeatures(self, features):
        self._data_original, cols_to_vars_dict = self.encode_features_dict(features)
        return self.__standardize_columns(self._data_original), cols_to_vars_dict

    #standardizes the values
    def __standardize_columns(self, x_original):
        std = x_original.std(dtype=np.float64, axis=0)
        mean = x_original.mean(dtype=np.float64, axis=0)
        x_standardized = (x_original - mean) / std
        self._standardized_max_values = np.amax(x_standardized, axis=0)
        self._standardized_min_values = np.amin(x_standardized, axis=0)
        return x_standardized

    #reverts the standardization
    def __un_standardize_columns(self, x_standardized, x_original):
        std = x_original.std(dtype=np.float64, axis=0)
        mean = x_original.mean(dtype=np.float64, axis=0)
        return mean + (x_standardized*std)

    def __getCsvHeader(self, filename):
        with open(filename, mode='r') as f:
            reader = csv.reader(f)
            header = next(reader)
            return header

    def __getCsvAsDict(self, filename):
        header = self.__getCsvHeader(filename)
        result = { h: [] for h in header }
        with open(filename, mode='r') as file:
            for row in csv.DictReader(file):
                for h in header:
                    result[h].append(row[h])

        return result
