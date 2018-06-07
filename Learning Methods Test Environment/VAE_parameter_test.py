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
    header = ["avg_cost","avg_un_dist","avg_n_dist","l_r", "n_h_1",
              "n_h_2", "n_z", "optimiz", "transf", "epochs", "batch",
             "n_cat", "n_num", "ammo_f", "train_log"]
    writer = csv.writer(csvfile, delimiter=',', quoting=csv.QUOTE_NONE)
    writer.writerow(header)

    return csvfile

csv_file = None;
cached_output = []
def write_to_csv(avg_cost, avg_un_dist, avg_n_dist, l_r, n_h_1, n_h_2, n_z, opt, tran, epochs,
                 batch, n_cat, n_num, ammo_f, train_log):
    global cached_output

    cached_output.append([avg_cost, avg_un_dist, avg_n_dist, l_r, n_h_1,
                     n_h_2, n_z, opt, tran, epochs, batch, n_cat,
                     n_num, ammo_f, train_log])

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

    avg_cost = 0.
    avg_distance_norm = 0.
    avg_distance_unnorm = 0.

    num_samples = test_data.num_examples

    for i in range(num_samples):
        batch = test_data.next_batch(1)

        x_reconstructed = network.encode_and_decode(batch)
        cost = network.calculate_loss(batch)

        _, unstandardized_batch = test_data.decode_processed_tensor(batch[0])
        _, unstandardized_x = test_data.decode_processed_tensor(x_reconstructed[0])

        distance_unnorm = tf.reduce_sum(tf.abs(unstandardized_batch - unstandardized_x))
        distance_norm = tf.reduce_sum(tf.abs(batch[0] - x_reconstructed[0]))

        distance_unnorm, distance_norm = sess.run((distance_unnorm, distance_norm))

        #compute average loss/cost
        avg_cost += cost / num_samples
        avg_distance_unnorm += distance_unnorm / num_samples
        avg_distance_norm += distance_norm / num_samples

    sess.close()
    avg_cost = "{:.5f}".format(avg_cost)
    avg_distance_unnorm = "{:.5f}".format(avg_distance_unnorm)
    avg_distance_norm = "{:.9f}".format(avg_distance_norm)

    return log, avg_cost, avg_distance_unnorm, avg_distance_norm


def start_model_training_and_write_results(learning_rate, n_hidden_1, n_hidden_2, n_z, batch_size, n_categorical,
                                          n_numerical, n_ammo_features, n_epochs, transfer_fct, optimizer):
    train_data, test_data = weapons.get_data(n_categorical, n_numerical, n_ammo_features)

    network_architecture = dict()
    network_architecture['n_input'] = train_data.num_features
    network_architecture['n_hidden_1'] = n_hidden_1
    network_architecture['n_hidden_2'] = n_hidden_2
    network_architecture['n_z'] = n_z

    opti = optimizer(learning_rate)
    train_log, avg_cost, avg_un_dist, avg_n_dist = train_model(train_data, test_data, network_architecture, opti, transfer_fct, batch_size, n_epochs, 1)

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

    write_to_csv(avg_cost, avg_un_dist, avg_n_dist, l_r, n_h_1, n_h_2, n_z, opt, tran,
                 epochs, batch, n_cat, n_num, ammo_f, train_log)

    gc.collect()
    tf.reset_default_graph()


def run_constellations_test(total_iterations, dry_run=False):
    iteration_count = 0
    total_iterations = total_iterations

    params = dict(learning_rate = 0.001,
                  n_hidden_1 = 10,
                  n_hidden_2 = 0, #not included
                  n_z = 4,
                  batch_size = 20,
                  n_categorical = 2, #constant
                  n_numerical = 15, #constant
                  n_ammo_features = 0, #constant
                  n_epochs = 20, #constant
                  transfer_fct = [tf.sigmoid, tf.tanh,
                                  tf.nn.elu,
                                  tf.nn.selu,
                                  tf.nn.softsign
                                  ],
                  optimizer =   [tf.train.AdamOptimizer,
                                 tf.train.RMSPropOptimizer,
                                 tf.train.FtrlOptimizer,
                                 tf.train.AdagradOptimizer
                                ]
                 )

    if not dry_run:
        printProgressBar(0, total_iterations, prefix = 'Progress:', suffix = 'Complete', length = 50, decimals = 3)

    learning_rate = params['learning_rate']
    #while True:
    #    learning_rate *= 0.1
    #    if learning_rate < 0.001:
    #        break

    n_hidden_1 = params['n_hidden_1']
    while True:
        n_hidden_1 -= 2
        if n_hidden_1 < 3:
            break
        '''
        n_hidden_2 = params['n_hidden_2']
        while True:
            n_hidden_2 -= 2
            if n_hidden_2 < 0:
                break
        '''
        n_z = params['n_z']
        while True:
            n_z -= 1
            if n_z < 1:
                break

            batch_size = params['batch_size']
            while True:
                batch_size *= 0.5
                batch_size = int(batch_size)
                if batch_size < 1:
                    break

                n_categorical = params['n_categorical']
                #while True:
                #    n_categorical -= 1
                #    if n_categorical < 0:
                #        break

                n_numerical = params['n_numerical']
                #    while True:
                #        n_numerical -= 1
                #        if n_numerical < 5:
                #            break

                n_ammo_features = params['n_ammo_features']
                #        while True:
                #            n_ammo_features -= 1
                #            if n_ammo_features < 0:
                #                break


                n_epochs = params['n_epochs']
                            #n_epochs = params['n_epochs']
                            #while True:
                                #n_epochs *= 0.5
                                #n_epochs = int(n_epochs)
                                #if n_epochs < 1:
                                    #break

                transfer_fct = params['transfer_fct']
                for fct in transfer_fct:

                    optimizer = params['optimizer']
                    for opt in optimizer:
                        iteration_count += 1
                        if not dry_run:
                            start_model_training_and_write_results(learning_rate, n_hidden_1, 0, n_z, batch_size, n_categorical,
                                                                    n_numerical, n_ammo_features, n_epochs, fct, opt)
                            printProgressBar(iteration_count, total_iterations, prefix = 'Progress:', suffix = 'Complete', length = 50, decimals = 3)

    if not dry_run:
        write_cache(close_file=True)
    return iteration_count

print("Start time = %s" %str(datetime.now()))
print("Gathering amount of possible constellations...")
total_iterations = run_constellations_test(1000000,dry_run=True)
print("Start test with %i different constellations" %total_iterations)
start_time = time.time()
run_constellations_test(total_iterations)
print("It took %s (dd:hh:mm:ss)" %(format_seconds(time.time()-start_time)))

#start_model_training_and_write_results(0.01, 14, 0, 2, 1, 2, 15, 0, 10, tf.nn.tanh, tf.train.AdamOptimizer)
#start_model_training_and_write_results(0.01, 14, 0, 2, 1, 2, 15, 0, 10, tf.nn.tanh, tf.train.RMSPropOptimizer)
#start_model_training_and_write_results(0.01, 14, 0, 2, 1, 2, 15, 0, 10, tf.nn.tanh, tf.train.FtrlOptimizer)
#start_model_training_and_write_results(0.01, 14, 0, 2, 1, 2, 15, 0, 10, tf.nn.tanh, tf.train.AdagradOptimizer)
#csv_file.close()
