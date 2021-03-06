#include<cstdio>
#include<cstring>
#include<cctype>
#include<cstdlib>
#include<fstream>
#include<sstream>
#include<map>
#include<vector>
#include<algorithm>

#include "utils.h"
#include "my_assert.h"
#include "GTFItem.h"
#include "Transcript.h"
#include "Transcripts.h"

using namespace std;

struct ChrInfo {
	string name;
	size_t len;

	ChrInfo(const string& name, size_t len) {
		this->name = name;
		this->len = len;
	}

	bool operator< (const ChrInfo& o) const {
		return name < o.name;
	}
};

int M;

vector<GTFItem> items;
vector<string> seqs;
vector<int> starts; // used to generate .grp
map<string, vector<int> > sn2tr; // map from seqname to transcripts
map<string, vector<int> >::iterator iter;
vector<ChrInfo> chrvec;

Transcripts transcripts;

char groupF[STRLEN], tiF[STRLEN], refFastaF[STRLEN];
char chromListF[STRLEN];

bool hasMappingFile;
char mappingFile[STRLEN];

map<string, string> mi_table; // mapping info table
map<string, string>::iterator mi_iter; //mapping info table's iterator

void loadMappingInfo(char* mappingF) {
	ifstream fin(mappingF);
	string line, key, value;

	general_assert(fin.is_open(), "Cannot open " + cstrtos(mappingF) + "! It may not exist.");

	mi_table.clear();
	while (getline(fin, line)) {
		line = cleanStr(line);
		if (line[0] == '#') continue;
		istringstream strin(line);
		strin>>value>>key;
		mi_table[key] = value;
	}

	fin.close();
}

bool buildTranscript(int sp, int ep) {
	int cur_s, cur_e; // current_start, current_end
	vector<Interval> vec;

	string transcript_id = items[sp].getTranscriptID();
	string gene_id = items[sp].getGeneID();
	string gene_name = "", transcript_name = "";
	
	char strand = items[sp].getStrand();
	string seqname = items[sp].getSeqName();
	string left = items[sp].getLeft();

	vec.clear();
	cur_s = cur_e = -1;
	for (int i = sp; i <= ep; i++) {
		int start = items[i].getStart();
		int end = items[i].getEnd();

		general_assert(strand == items[i].getStrand(), "According to the GTF file given, a transcript has exons from different orientations!");
		general_assert(seqname == items[i].getSeqName(), "According to the GTF file given, a transcript has exons on multiple chromosomes!");

		if (items[i].getGeneName() != "") {
		  if (gene_name == "") gene_name = items[i].getGeneName();
		  else general_assert(gene_name == items[i].getGeneName(), "A transcript is associated with multiple gene names!");
		}
		if (items[i].getTranscriptName() != "") {
		  if (transcript_name == "") transcript_name = items[i].getTranscriptName();
		  else general_assert(transcript_name == items[i].getTranscriptName(), "A transcript is associated with multiple transcript names!");
		}

		if (cur_e + 1 < start) {
			if (cur_s > 0) vec.push_back(Interval(cur_s, cur_e));
			cur_s = start;
		}
		cur_e = (cur_e < end ? end : cur_e);
	}
	if (cur_s > 0) vec.push_back(Interval(cur_s, cur_e));

	transcripts.add(Transcript(transcript_id, gene_id, seqname, strand, vec, left, transcript_name, gene_name));

	return true;
}

void parse_gtf_file(char* gtfF) {
	ifstream fin(gtfF);
	string line, tid, gid;
	GTFItem item;

	general_assert(fin.is_open(), "Cannot open " + cstrtos(gtfF) + "! It may not exist.");

	int cnt = 0;

	items.clear();
 	while (getline(fin, line)) {
 		if (line[0] == '#') continue; // if this line is comment, jump it
 		item.parse(line);
 		string feature = item.getFeature();
 		if (feature == "exon") {
 			if (item.getStart() > item.getEnd()) {
				printf("Warning: exon's start position is larger than its end position! This exon is discarded.\n");
 				printf("\t%s\n\n", line.c_str());
 			}
 			else if (item.getStart() < 1) {
				printf("Warning: exon's start position is less than 1! This exon is discarded.\n");
 				printf("\t%s\n\n", line.c_str());
 			}
 			else {
 				item.parseAttributes(line);
 		 		if (hasMappingFile) {
 		 			tid = item.getTranscriptID();
					mi_iter = mi_table.find(tid);
					general_assert(mi_iter != mi_table.end(), "Mapping Info is not correct, cannot find " + tid + "'s gene_id!");
					gid = mi_iter->second;
					item.setGeneID(gid);
 		 		}
 				items.push_back(item);
 			}
 		}

 		++cnt;
 		if (verbose && cnt % 200000 == 0) { printf("Parsed %d lines\n", cnt); }
	}
	fin.close();

	sort(items.begin(), items.end());

	int sp = 0, ep; // start pointer, end pointer
	int nItems = items.size();

	sn2tr.clear();
	while (sp < nItems) {
		tid = items[sp].getTranscriptID();

		ep = sp + 1;
		while (ep < nItems && items[ep].getTranscriptID() == tid) ep++;
		ep--;

		buildTranscript(sp, ep);

		int sid = transcripts.getM();
		const Transcript& transcript = transcripts.getTranscriptAt(sid);

		iter = sn2tr.find(transcript.getSeqName());
		if (iter == sn2tr.end()) {
			vector<int> vec(1, sid);
			sn2tr[transcript.getSeqName()] = vec;
		}
		else {
			iter->second.push_back(sid);
		}

		sp = ep + 1;
	}

	items.clear();

	M = transcripts.getM();
	general_assert(M > 0, "The reference contains no transcripts!");

	if (verbose) { printf("Parsing gtf File is done!\n"); }
}

inline char check(char c, string& seqname, int pos) {
	general_assert(isalpha(c), "Sequence " + seqname + " contains an unknown letter (ASCII code " + itos(c) + ") at 0-based position " + itos(pos) + "!");
	if (isupper(c) && c != 'A' && c != 'C' && c != 'G' && c != 'T') c = 'N';
	if (islower(c) && c != 'a' && c != 'c' && c != 'g' && c != 't') c = 'n';
	return c;
}

void shrink() {
  int curp = 0;

  for (int i = 1; i <= M; i++) 
    if (seqs[i] == "") {
      const Transcript& transcript = transcripts.getTranscriptAt(i);
      printf("Warning: Cannot extract transcript %s's sequence since the chromosome it locates, %s, is absent!\n", transcript.getTranscriptID().c_str(), transcript.getSeqName().c_str());
    }
    else {
      ++curp;
      transcripts.move(i, curp);
      if (i > curp) seqs[curp] = seqs[i];
    }

  printf("%d transcripts are extracted and %d transcripts are omitted.\n", curp, M - curp);

  transcripts.setM(curp);
  M = transcripts.getM();
  general_assert(M > 0, "The reference contains no transcripts!");

  starts.clear();
  string curgid = "", gid;

  for (int i = 1; i <= M; i++) {
    gid = transcripts.getTranscriptAt(i).getGeneID();
    if (curgid != gid) { 
      starts.push_back(i);
      curgid = gid;
    }
  }
  starts.push_back(M + 1);
}

void writeResults(char* refName) {
	int s;
	ofstream fout;

	sprintf(groupF, "%s.grp", refName);
	sprintf(tiF, "%s.ti", refName);
	sprintf(refFastaF, "%s.transcripts.fa", refName);
	sprintf(chromListF, "%s.chrlist", refName);


	fout.open(groupF);
	s = starts.size();
	for (int i = 0; i < s; i++) fout<<starts[i]<<endl;
	fout.close();
	if (verbose) { printf("Group File is generated!\n"); }

	transcripts.writeTo(tiF);
	if (verbose) { printf("Transcript Information File is generated!\n"); }

	fout.open(chromListF);
	s = chrvec.size();
	for (int i = 0; i < s; i++) {
		fout<<chrvec[i].name<<'\t'<<chrvec[i].len<<endl;
	}
	fout.close();
	if (verbose) { printf("Chromosome List File is generated!\n"); }

	fout.open(refFastaF);
	for (int i = 1; i <= M; i++) {
		fout<<">"<<transcripts.getTranscriptAt(i).getTranscriptID()<<endl;
		fout<<seqs[i]<<endl;
	}
	fout.close();
	if (verbose) { printf("Extracted Sequences File is generated!\n"); }
}

int main(int argc, char* argv[]) {
  if (argc < 6 || ((hasMappingFile = atoi(argv[4])) && argc < 7)) {
		printf("Usage: rsem-extract-reference-transcripts refName quiet gtfF hasMappingFile [mappingFile] chromosome_file_1 [chromosome_file_2 ...]\n");
		exit(-1);
	}

	verbose = !atoi(argv[2]);
	if (hasMappingFile) {
		loadMappingInfo(argv[5]);
	}
	parse_gtf_file(argv[3]);

	ifstream fin;
	string line, gseq, seqname;

	chrvec.clear();

	seqs.clear();
	seqs.resize(M + 1, "");
	int start = hasMappingFile ? 6 : 5;
	for (int i = start; i < argc; i++) {
		fin.open(argv[i]);
		general_assert(fin.is_open(), "Cannot open " + cstrtos(argv[i]) + "! It may not exist.");
		getline(fin, line);
		while ((fin) && (line[0] == '>')) {
			istringstream strin(line.substr(1));
			strin>>seqname;

			gseq = "";
			while((getline(fin, line)) && (line[0] != '>')) {
			  gseq += line;
			}

			size_t len = gseq.length();
			assert(len > 0);
			for (size_t j = 0; j < len; j++) gseq[j] = check(gseq[j], seqname, j);
			
			iter = sn2tr.find(seqname);
			if (iter == sn2tr.end()) continue;
			
			chrvec.push_back(ChrInfo(seqname, len));
			
			vector<int>& vec = iter->second;
			int s = vec.size();
			for (int j = 0; j < s; j++) {
			  assert(vec[j] > 0 && vec[j] <= M);
			  transcripts.getTranscriptAt(vec[j]).extractSeq(gseq, seqs[vec[j]]);
			}
		}
		fin.close();

		if (verbose) { printf("%s is processed!\n", argv[i]); }
	}
	sort(chrvec.begin(), chrvec.end());

	shrink();
	if (verbose) { printf("Extracting sequences is done!\n"); }

	writeResults(argv[1]);

	return 0;
}
