import warnings
import pandas as pd
import os
import sys
import tensorflow as tf
from datetime import datetime
from multiprocessing.pool import ThreadPool

warnings.filterwarnings('ignore')  # Suppress tensorflow logging
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'  # Suppress errors log


def classify(classifier, window):
    now = datetime.now()
    current_time = now.strftime("%H:%M:%S")
    labels = {
        0: 'free',
        1: 'washing',
        2: 'drying',
        3: 'soap'
    }

    # Classification
    prediction = classifier.predict(window)
    pred_label = prediction.argmax(1)[0]
    pred_porb = prediction[0][pred_label]

    # Log to console
    print(f'{current_time} --> {labels[pred_label]} (acc: {round(pred_porb, 2)})', file=sys.stderr)

    return str(':'.join([str(pred_label), str(round(pred_porb, 2))]))


if __name__ == '__main__':
    # Settings
    buffer = ''
    separator = ';'
    sampling_frequency = 9  # Hz
    time_window_size = sampling_frequency * 1  # s
    pool = ThreadPool(processes=1)
    df_realtime = pd.DataFrame()

    # Load the trained model
    model = tf.keras.models.load_model('model.h5')

    sys.stdout.buffer.write(bytes([0xca, 0xff, 0xee]))  # magic number
    sys.stdout.flush()

    while True:
        # Keep reading from stdin
        buffer += sys.stdin.readline()

        # Check if line is ready
        if buffer.endswith('\n'):
            line = buffer[:-1]
            buffer = ''

            # Add CSI amplitude value to create the sample to feed into the model
            time, mac, sub, amp, phase, rssi, fc = str(line).split(';')
            time = pd.to_datetime(str(pd.to_datetime(':'.join(time.split(':')[:-1]) + '.' + time.split(':')[-1])))
            df_realtime._set_value(time, col=f'sub ({sub})', value=float(amp.replace(',', '.')))

            # If the realtime data shape match the input shape required by the model then perform classification
            if df_realtime.shape[0] == time_window_size and not df_realtime.isnull().values.any():
                df_realtime = df_realtime.values.reshape((1, df_realtime.shape[0], df_realtime.shape[1]))

                # Prediction
                async_result = pool.apply_async(classify, (model, df_realtime))

                # Output prediction to stdout
                print('%s' % async_result.get())
                sys.stdout.flush()
                df_realtime = pd.DataFrame()
