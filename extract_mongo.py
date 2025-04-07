import os
import time
import io
import re
from tqdm import tqdm
from pymongo import MongoClient
from sys import argv

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
    return re.sub(r'[\r\n\t\0]', ' ', text).replace('', '').replace("  ", " ").strip()

# Export parallel corpus to text files
def process_batch(id_range, codes, corpora_dir, prefix=""):
    """
    Process multiple languages using aggregation pipeline
    
    Args:
        id_range: Tuple of (start_id, end_id)
        codes: List of language codes to process
        corpora_dir: Directory to save output files
        prefix: Prefix for output file names (default: "")
    """
    start_id, end_id = id_range
    
    # Build the lookup stages for each language after the first
    lookup_stages = []
    for code in codes[1:]:
        lookup_stages.append({
            '$lookup': {
                'from': code,
                'localField': '_id',
                'foreignField': '_id',
                'as': code
            }
        })
        lookup_stages.append({
            '$unwind': f'${code}'
        })
    
    # Build the project stage to rename fields
    project_fields = {'_id': 0}
    for code in codes:
        if code == codes[0]:
            project_fields[code] = f'${code}'
        else:
            project_fields[code] = f'${code}.{code}'
    
    # Create the aggregation pipeline with ID range
    pipeline = [
        {'$match': {'_id': {'$gte': start_id, '$lt': end_id}}},
        *lookup_stages,
        {'$project': project_fields}
    ]
    
    # Get count for this range
    count_pipeline = [
        {'$match': {'_id': {'$gte': start_id, '$lt': end_id}}},
        {'$count': 'total'}
    ]
    count_result = list(db[codes[0]].aggregate(count_pipeline))
    total_count = count_result[0]['total'] if count_result else 0
    
    if total_count == 0:
        return 0
    
    # Create buffers for all languages
    buffers = {code: io.StringIO() for code in codes}
    file_paths = {
        code: f"{corpora_dir}/{prefix}-{code}.txt" if prefix else f"{corpora_dir}/{code}.txt"
        for code in codes
    }
    
    processed_count = 0
    cursor = db[codes[0]].aggregate(pipeline, allowDiskUse=True)
    
    with tqdm(total=total_count, desc=f"Processing ID range {start_id}-{end_id}") as pbar:
        for doc in cursor:
            valid_doc = True
            texts = {}
            
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

    return processed_count

# Main processing
if __name__ == "__main__":
    if len(argv) < 3:
        print("Usage: python extract_mongo.py <prefix> <code1> <code2> ...")
        exit()

    prefix = argv[1]
    codes = argv[2:]
    
    # Get ID range from first collection
    min_id = db[codes[0]].find_one({}, sort=[('_id', 1)])['_id']
    max_id = db[codes[0]].find_one({}, sort=[('_id', -1)])['_id']
    print(f"ID range: {min_id} to {max_id}")
    
    corpora_dir = "corpora"
    os.makedirs(corpora_dir, exist_ok=True)

    # Clear all output files before starting
    print("Clearing output files...")
    for code in codes:
        output_file = f"{corpora_dir}/{prefix}{code}.txt"
        open(output_file, "w", encoding='utf8').close()

    start_time = time.time()
    total_processed = 0
    
    # Process in ranges of 100,000
    GAP = 100000
    for start_id in range(min_id, max_id + GAP, GAP):
        end_id = min(start_id + GAP, max_id + 1)
        processed_count = process_batch((start_id, end_id), codes, corpora_dir, prefix)
        total_processed += processed_count
        
    duration = time.time() - start_time

    print(f"\nExport complete:")
    print(f"Successfully exported {total_processed:,} parallel sentences")
    print(f"Time taken: {duration:.2f} seconds")
    print(f"Processing speed: {total_processed/duration:.2f} sentences/second")