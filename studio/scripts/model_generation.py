import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import tensorflow as tf
from statistics import mode
from imblearn.metrics import sensitivity_specificity_support
from keras.layers import Conv1D, Dropout, Flatten, Dense, MaxPool1D
from sklearn.metrics import confusion_matrix, precision_recall_fscore_support, accuracy_score
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error


def get_windows(df: pd.DataFrame, window_size, sampling_frequency: int, overlap):
    """
    This function segments the input dataset according to the passed parameters, it returns an array of segment both for
    data and labels.

    :param df: The dataframe to segment
    :param window_size: The size of the window (number of samples)
    :param sampling_frequency: The sampling frequency of the dataset, e.g. 10Hz
    :param overlap: Time of overlap between two windows
    :return:
    """
    hop_size = window_size - int(sampling_frequency * overlap)
    window = list()
    labels = list()

    for pos in range(0, len(df) - window_size + 1, hop_size):
        x_i = df[pos: pos + window_size]
        l_i = mode(x_i.pop('label'))

        window.append(x_i)
        labels.append(l_i)

    return np.asarray(window), np.asarray(labels)


def build_model(input_shape: int, output_shape: int):
    """
    Build a CNN model with given input and output shapes
    :param input_shape: number of inputs
    :param output_shape: number of class to predict
    :return:
    """
    model = tf.keras.Sequential()
    model.add(Conv1D(64, 5, activation='relu', input_shape=input_shape))
    model.add(Dropout(0.1))

    model.add(Conv1D(64, 5, activation='relu'))
    model.add(Dropout(0.2))

    model.add(MaxPool1D())

    model.add(Flatten())

    model.add(Dense(64, activation='relu'))
    model.add(Dropout(0.3))

    model.add(Dense(32, activation='relu'))
    model.add(Dropout(0.3))

    model.add(Dense(output_shape, activation='softmax'))

    model.compile(optimizer='adam',
                  loss=tf.losses.SparseCategoricalCrossentropy(from_logits=False),
                  metrics=['accuracy'])

    return model


def cm_analysis(y_true, y_pred, labels, title, lab_tag, ymap=None):
    """
    Generate matrix plot of confusion matrix with pretty annotations.
    The plot image is saved to disk.
    args:
      y_true:    true label of the raw-data, with shape (n_samples,)
      y_pred:    prediction of the raw-data, with shape (n_samples,)
      filename:  filename of figure file to save
      labels:    string array, name the order of class labels in the confusion matrix.
                 use `clf.classes_` if using scikit-learn models.
                 with shape (nclass,).
      ymap:      dict: any -> string, length == nclass.
                 if not None, map the labels & ys to more understandable strings.
                 Caution: original y_true, y_pred and labels must align.
      figsize:   the size of the figure plotted.
    """
    # Calc figure size based on number of class, for better visualisation
    tail_size = 2
    n_class = len(np.unique(y_true))
    figsize = (40, 30)
    if ymap is not None:
        y_pred = [ymap[yi] for yi in y_pred]
        y_true = [ymap[yi] for yi in y_true]
        labels = [ymap[yi] for yi in labels]
    cm = confusion_matrix(y_true, y_pred, labels=labels)
    cm_sum = np.sum(cm, axis=1, keepdims=True)
    cm_perc = cm / cm_sum.astype(float) * 100
    annot = np.empty_like(cm).astype(str)
    nrows, ncols = cm.shape
    for i in range(nrows):
        s = cm_sum[i]
        for j in range(ncols):
            c = cm[i, j]
            p = cm_perc[i, j]
            if i == j:
                s = cm_sum[i]
                annot[i, j] = '%.1f%%\n%d/%d' % (p, c, s)
            # elif c == 0:
            #     annot[i, j] = ''
            else:
                annot[i, j] = '%.1f%%\n%d' % (p, c)
    cm = pd.DataFrame(cm, index=lab_tag, columns=lab_tag)
    cm.index.name = 'Actual'
    cm.columns.name = 'Predicted'
    fig, ax = plt.subplots(figsize=figsize)
    plt.title('Confusion Matrix %s' % title)
    # sns.heatmap(cm, annot=annot, fmt='', ax=ax)
    sns.heatmap(cm, annot=annot, fmt='', cbar=False, cmap="BuGn", ax=ax)
    plt.ylabel('True label', fontweight='bold', fontsize=80)
    plt.xlabel('Predicted label', fontweight='bold', fontsize=80)
    plt.show()
    plt.close(fig)
    plt.close()


def print_stats(pred, true):
    """
    Prints out statistics on the predictions of the model
    :param pred: predicted labels
    :param true: ground truth
    :return:
    """
    metrics1 = sensitivity_specificity_support(true, pred, average='weighted')
    metrics2 = precision_recall_fscore_support(true, pred, average='weighted')
    metrics3 = accuracy_score(true, pred, normalize=True)

    print('• Specificity: %s\n'
          '• Sensitivity: %s\n'
          '• Precision: %s\n'
          '• Accuracy: %s\n'
          '• Recall: %s\n'
          '• F1-score: %s\n'
          '• Mean Squared Error: %s' % (metrics1[1], metrics1[0], metrics2[0], metrics3, metrics2[1], metrics2[0],
                                        mean_squared_error(true, pred))
          )


if __name__ == '__main__':
    # Set seed for experiment reproducibility
    seed = 28
    tf.random.set_seed(seed)
    np.random.seed(seed)

    # Settings
    sampling_frequency = 10
    seconds = 1
    window_size = sampling_frequency * seconds  # Hz
    overlap = 0
    training_epochs = 95
    time_window_size = '%ss' % seconds
    labels = ['free\nsink', 'washing\nhands', 'drying\nhands', 'soaping']
    path_to_file = 'path/to/file.csv'

    # Load the data
    df = pd.read_csv(path_to_file, index_col='timestamp', sep=';', parse_dates=True)
    df.dropna(inplace=True)
    num_classes = len(df['label'].unique())

    # Segmentation
    X, Y = get_windows(df, window_size, sampling_frequency, overlap)
    b, c = np.unique(Y, return_inverse=True)
    Y = c

    # Train and Test split
    X_train, X_test, y_train, y_test = train_test_split(X, Y, test_size=0.25, random_state=seed, stratify=Y)
    original_shape_train = X_train.shape
    original_shape_test = X_test.shape

    # Print some info
    print(
        'train %s\ntest %s\n# classes %s\ninput shape %s' %
        (X_train.shape, X_test.shape, num_classes, X_train[0].shape)
    )

    # Build the model
    model = build_model(X_train[0].shape, num_classes)

    # Train the model
    history = model.fit(X_train, y_train, validation_data=(X_test, y_test), epochs=training_epochs)

    # Test the model
    predicted = list((model.predict(X_test)).argmax(1))

    # Model evaluation performance
    print_stats(predicted, y_test)
    cm_analysis(y_test, predicted, [0, 1, 2, 3], '', labels)

    # Save model
    model.save('model.h5')
