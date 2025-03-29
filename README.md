# DeepTrans

Easy to use, cross-platform toolkit to train [deeptrans-train](https://github.com/wildwolf085/deamtrans-train) models, which can be used by [deeptrans](https://github.com/wildwolf085/deamtrans) 🚂

## Requirements

 * Python >= 3.11
 * NVIDIA CUDA graphics card (not required, but highly recommended)

## Install

```bash
git clone https://github.com/wildwolf085/deamtrans-train --depth 1
cd deamtrans-train
pip install -r requirements.txt
```

## Background

Language models can be trained by providing lots of example translations from a source language to a target language. All you need to get started is a set of two files (`source` and `target`). The source file containing sentences written in the source language and a corresponding file with sentences written in the target language.

For example:

`source.txt`:

```
Hello
I'm a train!
Goodbye
```

`target.txt`:

```
Hola
¡Soy un tren!
Adiós
```

## Usage

Place `source.txt` and `target.txt` files in a folder (e.g. `mydataset-en_es`) of your choice:

```bash
mydataset-en_es/
├── source.txt
└── target.txt
```

Create a `config.json` file specifying your sources:

```json
{
    "from": "en",
    "to": "es",
    "version": "1.0",
    "sources": [
        "file://D:\\path\\to\\mydataset-en_es",
        "opus://Ubuntu"
    ]
}
```

Note you can specify, local folders (using the `file://` prefix), internet URLs to .zip archives (using the `http://` or `https://` prefix) or [OPUS](https://opus.nlpl.eu/) datasets (using the `opus://` prefix). For a complete list of OPUS datasets, see [OPUS.md](OPUS.md) and note that they are case-sensitive.

Then run:

```bash
python train.py --config config.json
```

Training can take a while and depending on the size of datasets can require a graphics card with lots of memory.

The output will be saved in `run/[from]-[to]-[YYMMDD].[vocab_size(k)].[input_size(m)]..dp` ex `run/en-it-250228.32.100.dp` (trained on 32k vocab and 100m input pair sentences at 28/02/2025).

### Running out of memory

If you're running out of CUDA memory, decrease the `batch_size` parameter, which by default is set to `8192`:

```json
{
    "from": "en",
    "to": "es",
    "sources": [
        "file://D:\\path\\to\\mydataset-en_es"
    ],
    "batch_size": 2048
}
```

### Reverse Training

Once you have trained a model from `source => target`, you can easily train a reverse model `target => source` model by passing `--reverse`:

```bash
python train.py --config config.json --reverse
```

### Tensorboard

TensorBoard allows tracking and visualizing metrics such as loss and accuracy, visualizing the model graph and other features. You can enable tensorboard with the `--tensorboard` option:

```bash
python train.py --config config.json --tensorboard
```

### Tuning

The model is generated using sensible default values. You can override the [default configuration](https://github.com/wildwolf085/deamtrans-train/blob/main/train.py#L276) by adding values directly to your `config.json`. For example, to use a smaller dictionary size, add a `vocab_size` key in `config.json`:

```json
{
    "from": "en",
    "to": "es",
    "version": "0.1",
    "sources": [
        "file://D:\\path\\to\\mydataset-en_es"
    ],
    "vocab_size": 32000
}
```

## Evaluate

You can evaluate the model by running:

```bash
python eval.py --config config.json
Starting interactive mode
(en)> Hello!
(es)> ¡Hola!
(en)>
```

You can also compute [BLEU](https://en.wikipedia.org/wiki/BLEU) scores against the [flores200](https://github.com/facebookresearch/flores/blob/main/flores200/README.md) dataset for the model by running:

```bash
python eval.py --config config.json --bleu
BLEU score: 45.12354
```

To run evaluation:

```bash
python eval.py --config run/en_it-opus_1.0/config.json
```

## Contribute

Want to share your model with the world? Post it on [community.deeptrans.org](https://community.deeptrans.org) and we'll include in future releases of DeepTrans. 
Make sure to share both a forward and reverse model (e.g. `en => es` and `es => en`), otherwise we won't be able to include it in the model repository.

We also welcome contributions to DeepTrans! Just open a pull request.

## Use with DeepTrans

To install the resulting .argosmodel file, locate the `~/.local/share/argos-translate/packages` folder. On Windows this is the `%userprofile%\.local\share\argos-translate\packages` folder. Then create a `[from-code]_[to-code]` folder (e.g. `en_es`). If it already exists, delete or move it.

Extract the contents of the .argosmodel file (which is just a .zip file, you might need to change the extension to .zip) into this folder. Then restart DeepTrans.

You can also install .argosmodel packages from Python:
```
import pathlib
import argostranslate.package
package_path = pathlib.Path("/root/translate-en_it-2_0.argosmodel")
argostranslate.package.install_from_path(package_path)
```

## Credits

In no particular order, we'd like to thank:

 * [OpenNMT-py](https://github.com/OpenNMT/OpenNMT-py)
 * [SentencePiece](https://github.com/google/sentencepiece)
 * [OPUS](https://opus.nlpl.eu)
 * [DeepTrans](https://github.com/wildwolf085/deamtrans)

For making DeepTrans possible.

## License

AGPLv3


D:\appdata\env312\Scripts\activate.ps1

python train.py --source en --target ko --vocab 32000 --corpus_size 200000000 --dataset