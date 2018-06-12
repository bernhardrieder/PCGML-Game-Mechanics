# Copyright 2018 - Bernhard Rieder - All Rights Reserved.
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import time
from datetime import datetime
import csv
import tensorflow as tf
import variational_autoencoder as vae
import weapon_data as weapons
import gc
import numpy as np

tf.logging.set_verbosity(0)
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

def format_seconds(seconds):
    m, s = divmod(seconds, 60)
    h, m = divmod(m, 60)
    d, h = divmod(h, 24)

    return "%02d:%02d:%02d" % (h, m, s)

last_update_time = 0
#based on https://stackoverflow.com/questions/3173320/text-progress-bar-in-the-console
def printProgressBar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█'):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
    """
    global last_update_time
    deltaTime = time.time() - last_update_time
    remaining_time = " - Remaining time: %s" %format_seconds((total - iteration)*deltaTime)
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    it_progress = " (%i/%i)" %(iteration, total)
    print('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix + it_progress + remaining_time), end = '\r')

    last_update_time = time.time()
    # Print New Line on Complete
    if iteration == total:
        print()

def create_csv_file_and_write_header():
    date = datetime.now().strftime("%Y-%m-%d_%H-%M")
    path = "VAE_parameter_test/summary_"+date+".csv"
    csvfile = open(path, 'a', newline='')
    header = ["avg_cost_rand","avg_cost","avg_un_dist","avg_n_dist","l_r", "n_h_1",
              "n_h_2", "n_z", "optimiz", "transf", "epochs", "batch",
             "n_cat", "n_num", "ammo_f", "train_log"]
    writer = csv.writer(csvfile, delimiter=',', quoting=csv.QUOTE_NONE)
    writer.writerow(header)

    return csvfile

csv_file = None;
cached_output = []
def write_to_csv(avg_cost_rand, avg_cost, avg_un_dist, avg_n_dist, l_r, n_h_1, n_h_2, n_z, opt, tran, epochs,
                 batch, n_cat, n_num, ammo_f, train_log):
    global cached_output

    cached_output.append([avg_cost_rand, avg_cost, avg_un_dist, avg_n_dist, l_r, n_h_1,
                     n_h_2, n_z, opt, tran, epochs, batch, n_cat, n_num, ammo_f, train_log])

    if len(cached_output) % 10 == 0:
        write_cache()

def write_cache(close_file=False):
    global csv_file, cached_output
    if csv_file == None:
        csv_file = create_csv_file_and_write_header()
    for out in cached_output:
        text = ""
        for t in out:
            text += (t + ",")
        csv_file.write(text + "\n")

    cached_output[:] = []
    if close_file:
        csv_file.close()


def train_model(train_data, test_data, network_architecture, optimizer, transfer_fct, batch_size, num_epochs, epoch_debug_step):
    sess = tf.Session(graph=tf.get_default_graph())
    network, log = vae.get_new_trained(sess, train_data, network_architecture, optimizer,
                                   transfer_fct, batch_size, num_epochs, epoch_debug_step,
                                   get_log_as_string=True, save_model=False)

    avg_cost_rand = 0.
    avg_cost = 0.
    num_samples = test_data.num_examples

    total_batch = int(num_samples / batch_size)

    # Loop over all batches
    for i in range(total_batch):
        batch = test_data.next_batch(batch_size)
        cost = network.calculate_loss(batch)

        sample = np.random.uniform(low=test_data.standardized_min_values,
                                    high=test_data.standardized_max_values,
                                    size=(batch_size,network_architecture["n_input"]))
        cost_rand = network.calculate_loss(sample)

        #compute average loss/cost
        avg_cost_rand += min(cost_rand, 1000) / total_batch
        avg_cost += cost / total_batch

    sess.close()
    avg_cost_rand = "{:.2f}".format(avg_cost_rand)
    avg_cost = "{:.2f}".format(avg_cost)

    return log, avg_cost_rand, avg_cost


def start_model_training_and_write_results(learning_rate, n_hidden_1, n_hidden_2, n_z, batch_size, n_categorical,
                                          n_numerical, n_ammo_features, n_epochs, transfer_fct, optimizer):
    train_data, test_data = weapons.get_data(n_categorical, n_numerical, n_ammo_features)

    network_architecture = dict()
    network_architecture['n_input'] = train_data.num_features
    network_architecture['n_hidden_1'] = n_hidden_1
    network_architecture['n_hidden_2'] = n_hidden_2
    network_architecture['n_z'] = n_z

    opti = optimizer(learning_rate)
    train_log, avg_cost_random, avg_cost  = train_model(train_data, test_data, network_architecture, opti, transfer_fct, batch_size, n_epochs, 1)

    avg_un_dist = "N/A"
    avg_n_dist = "N/A"
    l_r = str(learning_rate)
    n_h_1 = str(n_hidden_1)
    n_h_2 = str(n_hidden_2)
    n_z = str(n_z)
    opt = optimizer.__name__
    tran = transfer_fct.__name__
    epochs = str(n_epochs)
    batch = str(batch_size)
    n_cat = str(n_categorical)
    n_num = str(n_numerical)
    ammo_f = str(n_ammo_features)

    write_to_csv(avg_cost_random, avg_cost, avg_un_dist, avg_n_dist, l_r, n_h_1, n_h_2, n_z, opt, tran,
                 epochs, batch, n_cat, n_num, ammo_f, train_log)

    gc.collect()
    tf.reset_default_graph()


def run_constellations_test(hyperparams, dry_run=False):
    iteration_count = 0

    if not dry_run:
        print("Gathering amount of possible constellations...")
        total_iterations = run_constellations_test(hyperparams, dry_run=True)
        print("Calculated %i different constellations" %total_iterations)
        printProgressBar(0, total_iterations, prefix = 'Progress:', suffix = 'Complete', length = 50, decimals = 3)

    for i in range(0,100):
        for learning_rate in hyperparams['learning_rate']:
            for n_hidden_1 in hyperparams['n_hidden_1']:
                for n_hidden_2 in hyperparams['n_hidden_2']:
                    for n_z in hyperparams['n_z']:
                        for batch_size in hyperparams['batch_size']:
                            for n_categorical in hyperparams['n_categorical']:
                                for n_numerical in hyperparams['n_numerical']:
                                    for n_ammo_features in hyperparams['n_ammo_features']:
                                        for n_epochs in hyperparams['n_epochs']:
                                            for transfer_fct in hyperparams['transfer_fct']:
                                                for optimizer in hyperparams['optimizer']:
                                                    iteration_count += 1
                                                    if not dry_run:
                                                        start_model_training_and_write_results(learning_rate, n_hidden_1, n_hidden_2, n_z, batch_size, n_categorical,
                                                                                                n_numerical, n_ammo_features, n_epochs, transfer_fct, optimizer)
                                                        printProgressBar(iteration_count, total_iterations, prefix = 'Progress:', suffix = 'Complete', length = 50, decimals = 3)

    if not dry_run:
        write_cache(close_file=True)
    else:
        return iteration_count

#__main__
hyperparams = dict( \
                    learning_rate = [0.01],
                    n_hidden_1 = [26],
                    n_hidden_2 = [12],
                    n_z = [2],
                    batch_size = [4],
                    n_categorical = [2],
                    n_numerical = [14],
                    n_ammo_features = [0],
                    n_epochs = [70],
                    #all possible nonlinear activation functions
                    transfer_fct = [
                                    #tf.tanh, #stick with that one
                                    tf.nn.elu, #has a pretty good (big) loss for random input with RMSPropOptimizer
                                    #tf.nn.selu, #has a pretty good (big) loss for random input with RMSPropOptimizer
                                    #tf.nn.softsign
                                    #tf.nn.softplus,
                                    #tf.sigmoid
                                   ],
                    optimizer = [
                                    #tf.train.AdamOptimizer, #stick with that one
                                    tf.train.RMSPropOptimizer
                                ]
                  )

print("Start time = %s" %str(datetime.now()))
start_time = time.time()
run_constellations_test(hyperparams)
print("It took %s (hh:mm:ss)" %(format_seconds(time.time()-start_time)))
