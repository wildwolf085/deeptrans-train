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

Language models can be trained by providing lots of example translations from a source language to a target language. All you need to get started is a set of two files (`{source_code}` and `target_code`). The source file containing sentences written in the source language and a corresponding file with sentences written in the target language.

For example:

`en.txt`:

```
Hello
Goodbye
```

`es.txt`:

```
Hola
Adiós
```

## Usage

Place `en.txt` and `es.txt` files in ".\corpora" folder:

```bash
corpora/
├── en.txt
└── es.txt
```

Then run:

```bash
python train.py en es

# [en] can be omitted.
python train.py es
```

Training can take a while and depending on the size of datasets can require a graphics card with lots of memory.

The output will be saved in `run/[from]-[to]-[YYMMDD].[vocab_size(k)].[input_size(m)]..dp` ex `run/en-it-250228.32.100.dp` (trained on 32k vocab and 100m input pair sentences at 28/02/2025).

### Reverse Training

Once you have trained a model from `source => target`, you can easily train a reverse model `target => source` model by passing `--reverse`:

```bash
python train.py en es --reverse
# [en] can be omitted.
python train.py es --reverse
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

python train.py ja

