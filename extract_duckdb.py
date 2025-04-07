import os
import time
import duckdb
from tqdm import tqdm
from sys import argv

# Constants
BATCH_SIZE = 100000

def process_files(directory: str, codes: list[str]):
    """
    Process multiple DuckDB files in parallel and export to text files
    
    Args:
        directory: Directory containing the DuckDB files
        codes: List of language codes to process
    """
    # Create DuckDB connections
    dbs = {}
    for code in codes:
        db_path = os.path.join(directory, f"{code}.db")
        if not os.path.exists(db_path):
            raise FileNotFoundError(f"Database not found: {db_path}")
        dbs[code] = duckdb.connect(db_path, read_only=True)
    
    # Open output files
    files = {
        code: open(os.path.join(directory, f"{code}.txt"), 'w', encoding='utf8', buffering=8192)
        for code in codes
    }
    
    # Get total count from first database (assuming all are aligned)
    total_rows = dbs[codes[0]].execute("SELECT COUNT(*) FROM texts").fetchone()[0]
    processed = 0
    
    print("Starting export...")
    start_time = time.time()
    
    try:
        # Process in batches
        for offset in range(0, total_rows, BATCH_SIZE):
            # Fetch batch from each database
            batches = {
                code: db.execute(
                    "SELECT text FROM texts ORDER BY id LIMIT ? OFFSET ?",
                    [BATCH_SIZE, offset]
                ).fetchall()
                for code, db in dbs.items()
            }
            
            # Write to files
            for i in range(len(batches[codes[0]])):
                for code in codes:
                    files[code].write(f"{batches[code][i][0]}\n")
            
            processed += len(batches[codes[0]])
            
            # Print progress
            duration = time.time() - start_time
            speed = processed / duration
            print(f"\rProcessed {processed:,}/{total_rows:,} documents. Speed: {speed:.2f} docs/sec", end="")
    
    finally:
        # Close all connections and files
        for db in dbs.values():
            db.close()
        for f in files.values():
            f.close()
    
    duration = time.time() - start_time
    return processed, duration

if __name__ == "__main__":
    if len(argv) < 3:
        print("Usage: python c_duckdb2txt.py <directory> <code1> <code2> ...")
        exit(1)
    
    directory = argv[1]
    codes = argv[2:]
    
    # Verify all databases exist
    for code in codes:
        db_path = os.path.join(directory, f"{code}.db")
        if not os.path.exists(db_path):
            print(f"Error: Database not found: {db_path}")
            exit(1)
    
    print(f"Processing databases from directory: {directory}")
    print(f"Language codes: {', '.join(codes)}")
    
    try:
        total_docs, duration = process_files(directory, codes)
        print(f"\nExport complete:")
        print(f"Successfully exported {total_docs:,} documents")
        print(f"Time taken: {duration:.2f} seconds")
        print(f"Average speed: {total_docs/duration:.2f} documents/second")
        
        # Print output file sizes
        for code in codes:
            txt_path = os.path.join(directory, f"{code}.txt")
            size_mb = os.path.getsize(txt_path) / (1024 * 1024)
            print(f"{code}.txt size: {size_mb:.2f} MB")
            
    except Exception as e:
        print(f"Error during export: {str(e)}")
        exit(1)