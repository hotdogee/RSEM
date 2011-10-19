README for RSEM
===============

[Bo Li](http://pages.cs.wisc.edu/~bli) \(bli at cs dot wisc dot edu\)

* * *

Table of Contents
-----------------

* [Introduction](#introduction)
* [Compilation & Installation](#compilation)
* [Usage](#usage)
* [Example](#example)
* [Simulation](#simulation)
* [Generate Transcript-to-Gene-Map from Trinity Output](#gen_trinity)
* [Acknowledgements](#acknowledgements)
* [License](#license)

* * *

## <a name="introduction"></a> Introduction

RSEM is a software package for estimating gene and isoform expression
levels from RNA-Seq data.  The new RSEM package (rsem-1.x) provides an
user-friendly interface, supports threads for parallel computation of
the EM algorithm, single-end and paired-end read data, quality scores,
variable-length reads and RSPD estimation. It can also generate
genomic-coordinate BAM files and UCSC wiggle files for visualization. In
addition, it provides posterior mean and 95% credibility interval
estimates for expression levels. 

## <a name="compilation"></a> Compilation & Installation

To compile RSEM, simply run
   
    make

To install, simply put the rsem directory in your environment's PATH
variable.

### Prerequisites

C++ and Perl are required to be installed. 

To take advantage of RSEM's built-in support for the Bowtie alignment
program, you must have [Bowtie](http://bowtie-bio.sourceforge.net) installed.

If you want to plot model learned by RSEM, you should also install R. 

## <a name="usage"></a> Usage

### I. Preparing Reference Sequences

RSEM can extract reference transcripts from a genome if you provide it
with gene annotations in a GTF file.  Alternatively, you can provide
RSEM with transcript sequences directly.

Please note that GTF files generated from the UCSC Table Browser do not
contain isoform-gene relationship information.  However, if you use the
UCSC Genes annotation track, this information can be recovered by
downloading the knownIsoforms.txt file for the appropriate genome.
 
To prepare the reference sequences, you should run the
'rsem-prepare-reference' program.  Run 

    rsem-prepare-reference --help

to get usage information or visit the [rsem-prepare-reference
documentation page](http://deweylab.biostat.wisc.edu/rsem/rsem-prepare-reference.html).

### II. Calculating Expression Values

To calculate expression values, you should run the
'rsem-calculate-expression' program.  Run 

    rsem-calculate-expression --help

to get usage information or visit the [rsem-calculate-expression
documentation page](http://deweylab.biostat.wisc.edu/rsem/rsem-calculate-expression.html).

#### Calculating expression values from single-end data

For single-end models, users have the option of providing a fragment
length distribution via the --fragment-length-mean and
--fragment-length-sd options.  The specification of an accurate fragment
length distribution is important for the accuracy of expression level
estimates from single-end data.  If the fragment length mean and sd are
not provided, RSEM will not take a fragment length distribution into
consideration.

#### Using an alternative aligner

By default, RSEM automates the alignment of reads to reference
transcripts using the Bowtie alignment program.  To use an alternative
alignment program, align the input reads against the file
'reference_name.idx.fa' generated by rsem-prepare-reference, and format
the alignment output in SAM or BAM format.  Then, instead of providing
reads to rsem-calculate-expression, specify the --sam or --bam option
and provide the SAM or BAM file as an argument.  When using an
alternative aligner, you may also want to provide the --no-bowtie option
to rsem-prepare-reference so that the Bowtie indices are not built.

However, please note that RSEM does ** not ** support gapped
alignments. So make sure that your aligner does not produce alignments
with intersions/deletions. Also, please make sure that you use
'reference_name.idx.fa' , which is generated by RSEM, to build your
aligner's indices.

### III. Visualization

RSEM contains a version of samtools in the 'sam' subdirectory. When
users specify the --out-bam option RSEM will produce three files:
'sample_name.bam', the unsorted BAM file, 'sample_name.sorted.bam' and
'sample_name.sorted.bam.bai' the sorted BAM file and indices generated
by the samtools included.

#### a) Generating a UCSC Wiggle file

A wiggle plot representing the expected number of reads overlapping
each position in the genome can be generated from the sorted BAM file
output.  To generate the wiggle plot, run the 'rsem-bam2wig' program on
the 'sample_name.sorted.bam' file.

Usage:    

    rsem-bam2wig bam_input wig_output wiggle_name

bam_input: sorted bam file   
wig_output: output file name, e.g. output.wig   
wiggle_name: the name the user wants to use for this wiggle plot  

#### b) Loading a BAM and/or Wiggle file into the UCSC Genome Browser

Refer to the [UCSC custom track help page](http://genome.ucsc.edu/goldenPath/help/customTrack.html).

#### c) Visualize the model learned by RSEM

RSEM provides an R script, 'rsem-plot-model', for visulazing the model learned.

Usage:
    
    rsem-plot-model sample_name outF

sample_name: the name of the sample analyzed    
outF: the file name for plots generated from the model. It is a pdf file    

The plots generated depends on read type and user configuration. It
may include fragment length distribution, mate length distribution,
read start position distribution (RSPD), quality score vs observed
quality given a reference base, position vs percentage of sequencing
error given a reference base and histogram of reads with different
number of alignments.

fragment length distribution and mate length distribution: x-axis is fragment/mate length, y axis is the probability of generating a fragment/mate with the associated length

RSPD: Read Start Position Distribution. x-axis is bin number, y-axis is the probability of each bin. RSPD can be used as an indicator of 3' bias

Quality score vs. observed quality given a reference base: x-axis is Phred quality scores associated with data, y-axis is the "observed quality", Phred quality scores learned by RSEM from the data. Q = -10log_10(P), where Q is Phred quality score and P is the probability of sequencing error for a particular base

Position vs. percentage sequencing error given a reference base: x-axis is position and y-axis is percentage sequencing error

Histogram of reads with different number of alignments: x-axis is the number of alignments a read has and y-axis is the number of such reads. The inf in x-axis means number of reads filtered due to too many alignments
 
## <a name="example"></a> Example

Suppose we download the mouse genome from UCSC Genome Browser.  We will
use a reference_name of 'mm9'.  We have a FASTQ-formatted file,
'mmliver.fq', containing single-end reads from one sample, which we call
'mmliver_single_quals'.  We want to estimate expression values by using
the single-end model with a fragment length distribution. We know that
the fragment length distribution is approximated by a normal
distribution with a mean of 150 and a standard deviation of 35. We wish
to generate 95% credibility intervals in addition to maximum likelihood
estimates.  RSEM will be allowed 1G of memory for the credibility
interval calculation.  We will visualize the probabilistic read mappings
generated by RSEM.

The commands for this scenario are as follows:

    rsem-prepare-reference --gtf mm9.gtf --mapping knownIsoforms.txt --bowtie-path /sw/bowtie /data/mm9 /ref/mm9
    rsem-calculate-expression --bowtie-path /sw/bowtie --phred64-quals --fragment-length-mean 150.0 --fragment-length-sd 35.0 -p 8 --out-bam --calc-ci --memory-allocate 1024 /data/mmliver.fq /ref/mm9 mmliver_single_quals
    rsem-bam2wig mmliver_single_quals.sorted.bam mmliver_single_quals.sorted.wig mmliver_single_quals

## <a name="simulation"></a> Simulation

### Usage: 

    rsem-simulate-reads reference_name estimated_model_file estimated_isoform_results theta0 N output_name [-q]

estimated_model_file:  file containing model parameters.  Generated by
rsem-calculate-expression.   
estimated_isoform_results: file containing isoform expression levels.
Generated by rsem-calculate-expression.   
theta0: fraction of reads that are "noise" (not derived from a transcript).   
N: number of reads to simulate.   
output_name: prefix for all output files.   
[-q] : set it will stop outputting intermediate information.   

### Outputs:

output_name.fa if single-end without quality score;   
output_name.fq if single-end with quality score;   
output_name_1.fa & output_name_2.fa if paired-end without quality
score;   
output_name_1.fq & output_name_2.fq if paired-end with quality score.   

output_name.sim.isoforms.results, output_name.sim.genes.results : Results estimated based on sample values.

## <a name="gen_trinity"></a> Generate Transcript-to-Gene-Map from Trinity Output

For Trinity users, RSEM provides a perl script to generate transcript-to-gene-map file from the fasta file produced by Trinity.

### Usage:

    extract-transcript-to-gene-map-from-trinity trinity_fasta_file map_file

trinity_fasta_file: the fasta file produced by trinity, which contains all transcripts assembled.    
map_file: transcript-to-gene-map file's name.    
 
## <a name="acknowledgements"></a> Acknowledgements

RSEM uses the [Boost C++](http://www.boost.org) and
[samtools](http://samtools.sourceforge.net) libraries.

## <a name="license"></a> License

RSEM is licensed under the [GNU General Public License v3](http://www.gnu.org/licenses/gpl-3.0.html).
