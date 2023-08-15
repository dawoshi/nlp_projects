#include <iostream>
#include <fstream>
#include <string>
#include <sys/time.h>
#include "base/logging.h"
#include "model/mrc_bert_ner.h"

using namespace std;
using namespace ner;

int main(){
  
  MrcBertNer detector;
  detector.Init();

  struct timeval t1, t2; 
  // gettimeofday(&t1, NULL);
  // std::string tmp;
  // std::string text = "财政部国家税务总局出台关于促进残疾人就业税收优惠政策的通知";
  // detector.predict(text, tmp);
  // std::cout << "predict: " << tmp << std::endl;
  // gettimeofday(&t2, NULL);
  // std::cout << "Totle run Time : " << ((t2.tv_sec - t1.tv_sec) +
  //   (double)(t2.tv_usec - t1.tv_usec)/1000000.0) *1000.0<< "ms" << std::endl;

  std::vector<string> courps;
  ifstream infile;
  infile.open("./test.txt");
  if (!infile.is_open())
    LOG(INFO) << "open file failure";
  while (!infile.eof()) {
    std::string line;
    while (getline(infile, line)) {
      courps.push_back(line);
    }   
  }   
  infile.close();
  LOG(INFO) << "test courps len count: " << courps.size();
  ofstream outfile;
  outfile.open("./msra_test.txt");
  if(!outfile.is_open()) {
      return -1;
  }
  double total_time = 0.0;
  for (size_t i = 0; i < courps.size(); ++i) {
    outfile << courps[i];
    outfile << "\t";
    gettimeofday(&t1, NULL);
    std::vector<std::string> tmp;
    detector.predict(courps[i], &tmp);
    gettimeofday(&t2, NULL);
    total_time += (t2.tv_sec - t1.tv_sec) +
      (double)(t2.tv_usec - t1.tv_usec)/1000000.0;
    for(int j = 0; j < tmp.size(); ++j) {
        outfile << tmp[j];
        outfile << "\n";
    }
  }
  outfile.close();  
  LOG(INFO) << "Totle run Time : " << total_time * 1000.0 << "ms";
  return 0;
}