import argparse
import random

def read_fasta(filename):
    with open(filename, 'r') as file:
        sequences = []
        sequence = ''
        header = ''
        for line in file:
            line = line.strip()
            if line.startswith('>'):
                if sequence:
                    sequences.append((header, sequence))
                header = line[1:]
                sequence = ''
            else:
                sequence += line
        if sequence:
            sequences.append((header, sequence))
        return sequences

def generate_random_quality(length):
    return ''.join(chr(random.randint(33, 73)) for _ in range(length))

def wrap_text(text, width=80):
    return '\n'.join([text[i:i+width] for i in range(0, len(text), width)])

def write_fastq(sequences, filename, width=80):
    with open(filename, 'w') as file:
        for header, sequence in sequences:
            quality = generate_random_quality(len(sequence))
            sequence_wrapped = wrap_text(sequence, width)
            quality_wrapped = wrap_text(quality, width)
            file.write(f'@{header}\n{sequence_wrapped}\n+\n{quality_wrapped}\n')

def main():
    parser = argparse.ArgumentParser(description='Convert FASTA to FASTQ format with random quality scores.')
    parser.add_argument('input_fasta', help='Path to the input FASTA file')
    parser.add_argument('output_fastq', help='Path to the output FASTQ file')
    args = parser.parse_args()

    sequences = read_fasta(args.input_fasta)
    write_fastq(sequences, args.output_fastq, width=80)

    print(f'{len(sequences)} sequences have been converted from FASTA to FASTQ format.')

if __name__ == '__main__':
    main()
