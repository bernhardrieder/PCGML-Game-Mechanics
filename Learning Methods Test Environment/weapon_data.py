#based on https://github.com/tensorflow/tensorflow/blob/r0.7/tensorflow/examples/tutorials/mnist/input_data.py
#based on https://github.com/tensorflow/tensorflow/blob/r1.8/tensorflow/contrib/learn/python/learn/datasets/mnist.py
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf
import csv

from tensorflow.python.framework import random_seed

DEFAULT_SOURCE_PATH = "./weapon_data.csv"
#define constants for data processing
CATEGORICAL_PARAMS  = ['type', 'firemode'] #exclude ammo because I'm not interested in decoding its values
#this numerical params are ordered by priority to omit less important ones in 'get_data()'
NUMERICAL_PARAMS = ['damages_first', 'damages_last', 'dmg_distances_last', 'rof',
                    'magsize', 'reloadempty', 'shotspershell', 'hiprecoilright', 'hiprecoilup', 'dmg_distances_first',
                    'initialspeed', 'hiprecoildec', 'drag', 'hordispersion', 'verdispersion' ]
WEAPON_TYPES = ['Shotgun', 'Pistol', 'Rifle', 'Submachine Gun', 'Sniper Rifle', 'Light Machine Gun']
WEAPON_FIREMODES = ['Automatic', 'Semi-Automatic', 'Single-Action']
NUM_OF_AMMO_TYPES = 31
EMBEDDED_AMMO_FEATURE_DIMENSION = 1


#quick'n'dirty convenience wrapper
def get_data(num_of_categorical_param, num_of_numerical_param, ammo_feature_column_dimension=EMBEDDED_AMMO_FEATURE_DIMENSION, seed=19071991, debug=False):
    include_ammo = False
    if num_of_categorical_param > len(CATEGORICAL_PARAMS):
        include_ammo = True
        num_of_categorical_param = len(CATEGORICAL_PARAMS)
    categorical = CATEGORICAL_PARAMS[0:num_of_categorical_param]

    if num_of_numerical_param > len(NUMERICAL_PARAMS):
        num_of_numerical_param = len(NUMERICAL_PARAMS)
    numerical = NUMERICAL_PARAMS[0:num_of_numerical_param]

    if ammo_feature_column_dimension == 0:
        ammo_feature_column_dimension = 1

    data = DataSet(categorical_params=categorical, include_ammo=include_ammo, numerical_params=numerical,
                    ammo_feature_column_dimension=ammo_feature_column_dimension, seed=seed, show_debug=False)

    if debug:
        print("Using %i categorical data:" %(len(categorical) if not include_ammo else len(categorical)+1 ))
        [print("\t"+x) for x in categorical]
        if include_ammo:
            print("\tammo")

        print("Using %i numerical data:" %len(numerical))
        [print("\t"+x) for x in numerical]

        param_sum=len(numerical)+len(categorical)
        if 'type' in categorical:
            param_sum += len(WEAPON_TYPES)-1
        if 'firemode' in categorical:
            param_sum += len(WEAPON_FIREMODES)-1
        if include_ammo:
            param_sum += ammo_feature_column_dimension
        print("That will sum up to %i parameters!" %param_sum)


    return data


def debug_printDict(dictionary):
    for key, value in dictionary.items():
        print(key, "=", value)

class DataSet:
    def __init__(self,
                 categorical_params,
                 include_ammo,
                 numerical_params,
                 ammo_feature_column_dimension,
                 data_source=DEFAULT_SOURCE_PATH,
                 seed=None,
                 show_debug=False):
        self._show_debug = show_debug
        seed1, seed2 = random_seed.get_seed(seed)
        np.random.seed(seed1 if seed is None else seed2)

        self._categorical_params = categorical_params
        self._include_ammo = include_ammo
        self._numerical_params = numerical_params
        self._ammo_feature_column_dimension = ammo_feature_column_dimension

        features = self.__getCsvAsDict(data_source)
        self._data, self._feature_cols_to_vars_dict = self.__encodeFeatures(features)

        self._num_examples, self._num_features = self.data.shape

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
    def epochs_completed(self):
        return self._epochs_completed

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
        '''Decodes a processed tensor and returns a dict'''

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
                for i in range(0, len(key[0][1])-1):
                    param = key[0][0] + "_" + key[0][1][i]
                    result[param] = str(unstandardized_tensor[idx])
                    idx += 1

            idx += 1
        return result

    def add_new_weapon(self, processed_tensor):
        self._data = np.append(self.data, [processed_tensor], axis=0)
        #self._data = np.concatenate((self.data[0, self._num_examples], processed_tensor), axis=0)
        self._num_examples = self._data.shape[0]
        if self._show_debug:
            print("Added new weapon/s! New number of examples in dataset = %i" %self._num_examples)

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

    def __encodeFeatures(self, features):
        #convert all numerical data to float so they can be used as 'tf.feature_column.numeric_column'
        for key, values in features.items():
            if key in self._numerical_params:
                features[key] = [float(value) for value in values]

        #create list where all feature columns come in
        columns = [tf.feature_column.numeric_column(param) for param in self._numerical_params]

        if 'type' in self._categorical_params:
            #one hot encoding of categorical data
            type_column = tf.feature_column.categorical_column_with_vocabulary_list(key='type', vocabulary_list=WEAPON_TYPES)
            type_column = tf.feature_column.indicator_column(type_column) #could use embedding column to further reduce dimensions
            columns.append(type_column)

        if 'firemode' in self._categorical_params:
            #one hot encoding of categorical data
            firemode_colum = tf.feature_column.categorical_column_with_vocabulary_list(key='firemode', vocabulary_list=WEAPON_FIREMODES)
            firemode_colum = tf.feature_column.indicator_column(firemode_colum) #could use embedding column to further reduce dimensions
            columns.append(firemode_colum)

        if self._include_ammo:
            #create this bucket just because i don't wnat to define the vocabulary list with every ammo name
            ammo_column = tf.feature_column.categorical_column_with_hash_bucket(key = "ammo", hash_bucket_size = NUM_OF_AMMO_TYPES)
            #actual dimension reduction with an embedding column
            ammo_column = tf.feature_column.embedding_column(ammo_column, dimension=self._ammo_feature_column_dimension)
            columns.append(ammo_column)

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

        self._data_original = np.array(weapon_data)
        return self.__standardize_columns(self._data_original), cols_to_vars_dict


    def __standardize_matrix(self, x_original):
        std = x_original.std(dtype=np.float64)
        mean = x_original.mean(dtype=np.float64)
        x_standardized = (x_original - mean) / std
        return x_standardized

    def __standardize_columns(self, x_original):
        std = x_original.std(dtype=np.float64, axis=0)
        mean = x_original.mean(dtype=np.float64, axis=0)
        x_standardized = (x_original - mean) / std
        return x_standardized

    def __un_standardize_matrix(self, x_standardized, x_original):
        std = x_original.std(dtype=np.float64)
        mean = x_original.mean(dtype=np.float64)
        return mean + (x_standardized*std)

    def __un_standardize_columns(self, x_standardized, x_original):
        std = x_original.std(dtype=np.float64, axis=0)
        mean = x_original.mean(dtype=np.float64, axis=0)
        return mean + (x_standardized*std)
