import argparse
import sys
import os
from datetime import datetime
import argparse
import zipfile
import shutil
import glob
import yaml
import subprocess
import re
import ctranslate2

from removedup import rdup
from fastshuffle import file_shuffle_sample

import sentencepiece as spm
from onmt_tools import average_models, sp_vocab_to_onmt_vocab
from languages import languages

usage = """
Usage: python train.py <from_code> <to_code> [additional arguments]

    additional arguments:
    --vocab_size vocab_size. default 32,000
    --corpus_size corpus_size. default 100,000,000
    --reverse. reverse the source and target languages in the configuration and data sources. default False
    --restart. restart the training from scratch. default False
    --build. while training is in progress on a separate process, you can launch another instance of train.py with this flag turned on to build a model from the last available checkpoints rather that waiting until the end. default False
"""

parser = argparse.ArgumentParser(description='Train DeepTrans models')
parser.add_argument("pair",nargs="*")
parser.add_argument('--vocab_size',
    type=int,
    default=32000,
    help='Vocabulary size. Default: %(default)s')
parser.add_argument('--corpus_size',
    type=int,
    default=72000000 * 2,
    help='Corpus size. Default: %(default)s')
parser.add_argument('--reverse',
    action='store_true',
    default=False,
    help='Reverse the source and target languages in the configuration and data sources. Default: %(default)s')
parser.add_argument('--bpe',
    action='store_true',
    default=False,
    help='Use BPE tokernize instead of unigram. Default: %(default)s')
parser.add_argument('--restart',
    action='store_true',
    default=False,
    help='Restart the training from scratch. Default: %(default)s')
parser.add_argument('--test',
    action='store_true',
    default=False,
    help='Train a test model (useful for testing). Default: %(default)s')
parser.add_argument('--shuffle',
    action='store_true',
    default=False,
    help='Shuffle data Default: %(default)s')
parser.add_argument('--build',
    action='store_true',
    help='While training is in progress on a separate process, you can launch another instance of train.py with this flag turned on to build a model from the last available checkpoints rather that waiting until the end. Default: %(default)s')

args = parser.parse_args() 

test = args.test
shuffle_only = args.shuffle
reverse = args.reverse
build = args.build
restart = args.restart
use_bpe = args.bpe
sp_name = "bpe" if use_bpe else "sp"

# calculate num_threads from cpu count
num_threads = os.cpu_count() - 2
if num_threads < 2: num_threads = 2
if num_threads > 32: num_threads = 32
print(f"num_threads: {num_threads}")

if test:
    from_code = "en"
    to_code = "zh"
    vocab_size, corpus_size = 2500, 5000
else:
    pair = args.pair
    if len(pair)==1:
        from_code = 'en'
        to_code = pair[0]
    elif len(pair)==2:
        from_code = pair[0]
        to_code = pair[1]
    else:
        print("Error: Please provide a valid language pair. Example: python train.py en zh")
        exit(1)
    
    vocab_size, corpus_size = args.vocab_size, args.corpus_size

if reverse: from_code, to_code = to_code, from_code

current_dir = os.path.dirname(__file__)
corpora_dir = os.path.join(current_dir, "test-corpora" if test else "corpora")

avg_checkpoints = 1


character_coverage = 1.0
seq_length = 5000

enc_layers = 4 if test else 6
dec_layers = 4 if test else 6
heads = 4 if test else 8
hidden_size = 368 if test else 512
word_vec_size = 368 if test else 512
valid_steps = 200 if test else 5000
train_steps = 1000 if test else 150000
save_checkpoint_steps = 100 if test else valid_steps
keep_checkpoint = 5

_date_code = datetime.today().strftime('%y%m')
_vocab_size = round(vocab_size / 1000)
_corpus_size = round(corpus_size / 1000000)
from_file = f"{corpora_dir}/{from_code}.txt"
to_file = f"{corpora_dir}/{to_code}.txt"

_tokenizer = ".bpe" if use_bpe else ""

version = f"{_vocab_size}k.{_corpus_size}m{_tokenizer}.{_date_code}"

valid_languages = [f[:-4] for f in filter(lambda x: x.endswith('.txt'), os.listdir(corpora_dir))]

if not os.path.exists(from_file):
    print(f"Valid language codes: {valid_languages}")
    exit(1)

if not os.path.exists(to_file):
    print(f"Valid language codes: {valid_languages}")
    exit(1)

model_name = f"{from_code}-{to_code}-{version}"
run_dir = os.path.join(current_dir, "run", model_name)
onmt_dir = os.path.join(run_dir, "opennmt")

if restart and os.path.isdir(run_dir): shutil.rmtree(run_dir)
os.makedirs(run_dir, exist_ok=True)


print(f"Training {languages[from_code]['en']} --> {languages[to_code]['en']} (tag: {version})")

if not os.path.isfile(os.path.join(run_dir, f'{from_code}.txt')):
    src_train = os.path.join(run_dir, f"{from_code}.txt")
    tgt_train = os.path.join(run_dir, f"{to_code}.txt")

    print("Writing shuffled sets")
    os.makedirs(run_dir, exist_ok=True)

    src, tgt, src_sample, tgt_sample = file_shuffle_sample(from_file, to_file, 1000 if test else 5000)
    os.rename(src, src_train)
    os.rename(tgt, tgt_train)
    os.rename(src_sample, os.path.join(run_dir, f"{from_code}-val.txt"))
    os.rename(tgt_sample, os.path.join(run_dir, f"{to_code}-val.txt"))
    
    print("Removing duplicates")
    src, tgt, removed = rdup(src_train, tgt_train)
    print(f"Removed {removed} lines")
    os.unlink(src_train)
    os.unlink(tgt_train)
    os.rename(src, src_train)
    os.rename(tgt, tgt_train)

if shuffle_only:
    print("done")
    exit(0)

os.makedirs(onmt_dir, exist_ok=True)
sp_model_path = os.path.join(run_dir, f"{sp_name}.model")
if not os.path.isfile(sp_model_path):
    try:
        spm.SentencePieceTrainer.train(
            model_type="bpe" if use_bpe else "unigram",
            input=[from_file, to_file], 
            model_prefix=f"{run_dir}/{sp_name}", 
            vocab_size=vocab_size,
            character_coverage=character_coverage,
            input_sentence_size=corpus_size,
            train_extremely_large_corpus=True,
            max_sentence_length=8192,
            num_threads=num_threads,
            shuffle_input_sentence=True
        )
    except Exception as e:
        print(str(e))
        exit(1)

transforms = ['sentencepiece', 'filtertoolong']

onmt_config = {
    'log_file': f'{run_dir}/opennmt.log',
    'save_data': onmt_dir,
    'src_vocab': f"{onmt_dir}/openmt.vocab",
    'tgt_vocab': f"{onmt_dir}/openmt.vocab",
    'src_vocab_size': vocab_size,
    'tgt_vocab_size': vocab_size,
    'share_vocab': True, 
    'data': {
        'train': {
            'path_src': f'{run_dir}/{from_code}.txt',
            'path_tgt': f'{run_dir}/{to_code}.txt',
            'transforms': transforms,
            'weight': 1
        },
        'valid': {
            'path_src': f'{run_dir}/{from_code}-val.txt',
            'path_tgt': f'{run_dir}/{to_code}-val.txt', 
            'transforms': transforms
        }
    }, 
    'src_subword_type': "bpe" if use_bpe else 'sentencepiece',
    'tgt_subword_type': "bpe" if use_bpe else 'sentencepiece',
    'src_onmttok_kwargs': {
        'mode': 'none',
        'lang': from_code,
    },
    'tgt_onmttok_kwargs': {
        'mode': 'none',
        'lang': to_code,
    },
    'src_subword_model': f'{run_dir}/{sp_name}.model', 
    'tgt_subword_model': f'{run_dir}/{sp_name}.model', 
    'src_subword_nbest': 1, 
    'src_subword_alpha': 0.0, 
    'tgt_subword_nbest': 1, 
    'tgt_subword_alpha': 0.0, 
    'src_seq_length': seq_length, 
    'tgt_seq_length': seq_length, 
    'skip_empty_level': 'silent', 
    'save_model': f'{onmt_dir}/openmt.model', 
    'save_checkpoint_steps': save_checkpoint_steps, 
    'keep_checkpoint': keep_checkpoint,
    'valid_steps': valid_steps, 
    'train_steps': train_steps, 
    'early_stopping': 4, 
    'bucket_size': 262144, 
    'num_worker': 4, # one GPU 4, two GPUs 2. # original 2,
    'world_size': 1, 
    'gpu_ranks': [0], 
    'batch_type': 'tokens', 
    'queue_size': 10000,
    'batch_size': 8192,
    'valid_batch_size': 2048,
    'max_generator_batches': 2, 
    'accum_count': 8, 
    'accum_steps': 0, 
    'model_dtype': 'fp16', 
    'optim': 'adam', 
    'learning_rate': 0.15,
    'warmup_steps': 16000, 
    'decay_method': 'rsqrt', 
    'adam_beta2': 0.998, 
    'max_grad_norm': 0, 
    'label_smoothing': 0.1, 
    'param_init': 0, 
    'param_init_glorot': True, 
    'normalization': 'tokens', 
    'encoder_type': 'transformer', 
    'decoder_type': 'transformer', 
    'position_encoding': True,
    # 'max_relative_positions': 20,
    'enc_layers': enc_layers, 
    'dec_layers': dec_layers,
    'heads': heads,
    'hidden_size': hidden_size,
    'rnn_size': hidden_size,
    'word_vec_size': word_vec_size, 
    'dropout': 0.1,
    'attention_dropout': 0.1,
    'share_decoder_embeddings': True,
    'share_embeddings': True,
    'valid_metrics': ['BLEU'],
}

if sys.platform == 'darwin' or ctranslate2.get_cuda_device_count() == 0:
    # CPU
    del onmt_config['gpu_ranks']

onmt_config_path = os.path.join(run_dir, "config.yml")
with open(onmt_config_path, "w", encoding="utf-8") as f:
    f.write(yaml.dump(onmt_config))
    print(f"Wrote {onmt_config_path}")

sp_vocab_file = os.path.join(run_dir, f"{sp_name}.vocab")
onmt_vocab_file = os.path.join(onmt_dir, "openmt.vocab")
    
if not os.path.isfile(onmt_vocab_file):
    sp_vocab_to_onmt_vocab(sp_vocab_file, onmt_vocab_file)

last_checkpoint = os.path.join(onmt_dir, os.path.basename(onmt_config["save_model"]) + f'_step_{onmt_config["train_steps"]}.pt')

def get_checkpoints():
    chkpts = [cp for cp in glob.glob(os.path.join(onmt_dir, "*.pt")) if "averaged.pt" not in cp]
    return list(sorted(chkpts, key=lambda x: int(re.findall('\d+', x)[0])))

if restart or (not (os.path.isfile(last_checkpoint) or build)):
    cmd = ["onmt_train", "-config", onmt_config_path]

    if restart:
        delete_checkpoints = glob.glob(os.path.join(onmt_dir, "*.pt"))
        for dc in delete_checkpoints:
            os.unlink(dc)
            print(f"Removed {dc}")

    # Resume?
    checkpoints = get_checkpoints()
    if len(checkpoints) > 0:
        print(f"Resuming from {checkpoints[-1]}")
        cmd += ["--train_from", checkpoints[-1]]

    subprocess.run(cmd)

# Average
average_checkpoint = os.path.join(run_dir, "averaged.pt")
checkpoints = get_checkpoints()
print(f"Total checkpoints: {len(checkpoints)}")

if len(checkpoints) == 0:
    print("Something went wrong, looks like onmt_train failed?")
    exit(1)

if os.path.isfile(average_checkpoint):
    os.unlink(average_checkpoint)

if len(checkpoints) == 1 or build:
    print("Single checkpoint")
    average_checkpoint = checkpoints[-1]
else:
    if avg_checkpoints == 1:
        print("No need to average 1 model")
        average_checkpoint = checkpoints[-1]
    else:
        avg_num = min(avg_checkpoints, len(checkpoints))
        print(f"Averaging {avg_num} models")
        average_models(checkpoints[-avg_num:], average_checkpoint)
# Quantize
ct2_model_dir = os.path.join(run_dir, "model")
if os.path.isdir(ct2_model_dir): shutil.rmtree(ct2_model_dir)

print("Converting to ctranslate2")
subprocess.run(
    [
        "ct2-opennmt-py-converter",
        "--model_path",
        average_checkpoint,
        "--output_dir",
        ct2_model_dir,
        "--quantization",
        "int8"
    ]
)

# Create .deeptrans package
package_file = os.path.join(run_dir, f"{model_name}.zip")
if os.path.isfile(package_file):
    os.unlink(package_file)
print(f"Writing {package_file}")

with zipfile.ZipFile(package_file, 'w') as zipf:
    files = os.listdir(ct2_model_dir)
    for file in files:
        zipf.write(os.path.join(ct2_model_dir, file), file)
    
    zipf.write(sp_model_path, f"{sp_name}.model")

print("Done!")
