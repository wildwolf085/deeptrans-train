import sys
import os
import sentencepiece as spm
from data import merge_shuffle


# corpora_dir = "e:\\corpora"
# with open("e:\\corpora\\en.corpus", "r", encoding="utf-8") as f:
    
# print(line_count)  # Should output ~80,000,000
# # output is 71366958
# exit(0)
# source = {
#     'source': f'{corpora_dir}/en.corpus',
#     'target': f'{corpora_dir}/zh.corpus',
#     "weight": None,
#     "filters": [],
#     "transforms": [],
#     "augmenters": []
# }

# print("start merge shuffle")

# merge_shuffle(
#     {
#         "corpus_1": source
#     },
#     "corpora",
#     max_eval_sentences=5000,
#     remove_duplicates=True
# )

lang = sys.argv[1]
print(lang)

current_dir = os.path.dirname(__file__)
tokenizer_dir = f"{current_dir}/tokenizer"
source_dir = f"{current_dir}/corpora"

os.makedirs(tokenizer_dir, exist_ok=True)

input_file = os.path.join(source_dir, f"{lang}.txt")  # Correct path handling

print("start train BPE tokenizer")
# https://github.com/google/sentencepiece/blob/master/doc/options.md
spm.SentencePieceTrainer.train(
    input=input_file,
    model_type="bpe",
    model_prefix=f"{tokenizer_dir}/{lang}",
    vocab_size=16000,
    character_coverage=1.0,
    input_sentence_size= 0, # 100000000,
    train_extremely_large_corpus=True,
    shuffle_input_sentence=True,
    max_sentence_length=8192,
    num_threads=16
)

""" 
tokenizer training log (unigram)
Step 400/100000; acc: 90.1; ppl: 5.4; xent: 1.7; lr: 0.00119; sents:  110319; bsz: 7829/2776/276; 8432/2990 tok/s; 4101 sec;
Step 450/100000; acc: 90.4; ppl: 5.5; xent: 1.7; lr: 0.00119; sents:  107571; bsz: 7764/2789/269; 5850/2102 tok/s; 4632 sec;

Metric	            | Example Value	        | Meaning
----------------------------------------------------------------------------------------------------------------
sub_iter	        | 0 or 1	            | Sub-iteration within an EM iteration (0=pruning phase, 1=optimization)
size	            | 248602 → 78658	    | Current vocabulary size during iterative pruning
obj	                | 79.6503 → 83.2406	    | Objective function value (higher = better model likelihood)
num_tokens	        | 1B+	                | Total tokens processed in this iteration
num_tokens/piece	| 4053 → 14391	        | Average token count per vocabulary item


BPE tokenizer training log
-------------------------------------------------------------------------------------------------------------------
bpe_model_trainer.cc(159) LOG(INFO) Updating active symbols. max_freq=2831 min_freq=186
bpe_model_trainer.cc(268) LOG(INFO) Added: freq=2829 size=37920 all=2895024 active=145186 piece=Γûütheological
bpe_model_trainer.cc(268) LOG(INFO) Added: freq=2826 size=37940 all=2895919 active=146081 piece=Γûüreporters
bpe_model_trainer.cc(268) LOG(INFO) Added: freq=2822 size=37960 all=2896568 active=146730 piece=ΓûüRicardo
bpe_model_trainer.cc(268) LOG(INFO) Added: freq=2820 size=37980 all=2897219 active=147381 piece=ΓûüIntermediate
bpe_model_trainer.cc(268) LOG(INFO) Added: freq=2817 size=38000 all=2898018 active=148180 piece=herita
bpe_model_trainer.cc(159) LOG(INFO) Updating active symbols. max_freq=2817 min_freq=185
bpe_model_trainer.cc(268) LOG(INFO) Added: freq=2815 size=38020 all=2898736 active=145614 piece=Γûüpristine
bpe_model_trainer.cc(268) LOG(INFO) Added: freq=2812 size=38040 all=2899674 active=146552 piece=Γûüreplicas
trainer_interface.cc(686) LOG(INFO) Saving model: D:\deamtrans-locomotive/tokenizers/en.model
trainer_interface.cc(698) LOG(INFO) Saving vocabs: D:\deamtrans-locomotive/tokenizers/en.vocab

"""

print("Done!")
