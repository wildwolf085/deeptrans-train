# import os
# import time
import io
import re
from tqdm import tqdm
from pymongo import MongoClient
# from sys import argv

client = MongoClient("mongodb://localhost:27017")
db = client["cp-250227"]

collections = db.list_collection_names()

# d_en = db["en"]
""" SchemaCorpus
    _id: number
    [language_code]: string
"""

# remove \r \n and \t, and trim spaces, multiple spaces to one space, null to empty string
def sanitize(text):
    return re.sub(r'[\r\n\t\0]', ' ', text).replace("  ", " ", 1000).strip()

# Main processing
if __name__ == "__main__":
    # Get count for this range
    count = db['zh_2'].count_documents({})
    
    codes = ['en', 'zh']
    corpora_dir = "corpora"
    filename = "c"
    # Create buffers for all languages

    buffers = {code: io.StringIO() for code in codes}
    file_paths = { code: f"{corpora_dir}/{filename}.{code}" for code in codes }
    for code in codes: open(file_paths[code], "w", encoding='utf8', buffering=8192).close()

    # Process the entire collection
    processed_count = 0
    cursor = db['zh_2'].find({}, {code: 1 for code in codes}).sort('_id', 1)
    
    with tqdm(total=count, desc=f"Processing ID range {1}-{count}") as pbar:
        for doc in cursor:
            valid_doc = True
            texts = {}
            
            # Extract and sanitize texts
            for code in codes:
                text = doc.get(code)
                if not text:
                    valid_doc = False
                    break
                texts[code] = sanitize(text)
            
            if valid_doc:
                for code in codes:
                    buffers[code].write(f"{texts[code]}\n")
                processed_count += 1
            
            pbar.update(1)
            
            if processed_count % 10000 == 0:
                for code in codes:
                    with open(file_paths[code], "a", encoding='utf8', buffering=8192) as f:
                        f.write(buffers[code].getvalue())
                    buffers[code].seek(0)
                    buffers[code].truncate()

    # Write remaining records
    for code in codes:
        if buffers[code].tell() > 0:
            with open(file_paths[code], "a", encoding='utf8', buffering=8192) as f:
                f.write(buffers[code].getvalue())
        buffers[code].close()

    print(f"\nExport complete:")
    print(f"Successfully exported {processed_count:,} parallel sentences")
