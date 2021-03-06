#include <Rcpp.h>
#include "read.h"
#include <ncbi-vdb/NGS.hpp>
#include <ngs-bam/ngs-bam.hpp>
#include <ngs/ErrorMsg.hpp>
#include <ngs/ReadCollection.hpp>
#include <ngs/ReadIterator.hpp>
#include <ngs/Read.hpp>


#include <math.h>
#include <iostream>

using namespace ngs;
using namespace std;
using namespace Rcpp;



//' The readCount in the read collection.
//'
//' This simply returns the full read count.
//'
//' @param acc An accession or a path to an actual SRA file (with .sra suffix)
//' @return the number of reads in the collection
//' @export
//' @examples
//' getFastqCount('SRR000123')
// [[Rcpp::export]]
long getFastqCount(Rcpp::String acc, bool forward_to_r = true) {
  try {
    ReadCollection run = ncbi::NGS::openReadCollection ( acc );
    long MAX_ROW = run.getReadCount ();
    return MAX_ROW;
  } catch(std::exception &ex) {
    if (forward_to_r) {
      forward_exception_to_r(ex);
    }
    return -1;
  } catch(...) { 
    ::Rf_error("c++ exception (unknown reason)"); 
    return -1;
  }
}


//' The reads in the read collection.
//'
//' This returns ALL reads or max_num_reads (with default of 0 meaning ALL reads).
//'
//' @param acc An accession or a path to an actual SRA file (with .sra suffix)
//' @param n The number of reads to return (default of 0 for ALL reads)
//' @return the reads in the collection
//' @export
//' @examples
//' getFastqReads('SRR000123',10)
// [[Rcpp::export]]
Rcpp::List getFastqReads(Rcpp::String acc, long max_num_reads = 0) {
  try {
    ReadCollection run = ncbi::NGS::openReadCollection ( acc );
    
    if (max_num_reads<1) { max_num_reads = run.getReadCount (); }
    
    ReadIterator rgi = run.getReads( Read::all );
    
    vector<std::string> out;
    for(int i = 0; rgi.nextRead() && i<max_num_reads; i++) {
      while ( rgi.nextFragment() ) {
        out.push_back(rgi.getFragmentBases().toString());
      }
    }
    return List::create (
        _["reads"] = out
    );
  } catch(std::exception &ex) {	
    forward_exception_to_r(ex);
    return -1;
  } catch(...) { 
    ::Rf_error("c++ exception (unknown reason)"); 
    return -1;
  }
}

//' The reads in the read collection.
//'
//' This returns the all reads.
//'
//' @param acc An accession or a path to an actual SRA file (with .sra suffix)
//' @param n The number of reads to return
//' @return the reads in the collection
//' @export
//' @examples
//' getFastqReadsWithQuality('SRR000123',10)
// [[Rcpp::export]]
Rcpp::List getFastqReadsWithQuality(Rcpp::String acc, long max_num_reads = 0) {
  try {
    ReadCollection run = ncbi::NGS::openReadCollection ( acc );
    
    if (max_num_reads<1) { max_num_reads = run.getReadCount (); }
    
    ReadIterator rgi = run.getReads( Read::all );
    
    vector<std::string> reads;
    vector<std::string> qualities;
    
    
    for(int i = 0; rgi.nextRead() && i<max_num_reads; i++) {
      if((i % 100000) == 0) {
        Rcpp::checkUserInterrupt();
      }
      while ( rgi.nextFragment() ) {
        reads.push_back(rgi.getFragmentBases().toString());
        qualities.push_back(rgi.getFragmentQualities().toString());
      }
    }
    return List::create (
        _["reads"] = reads,
        _["qualities"] = qualities  
    );
  } catch(std::exception &ex) {	
    forward_exception_to_r(ex);
    return (Rcpp::List());
  } catch(...) { 
    ::Rf_error("c++ exception (unknown reason)"); 
    return -1;
  }
}   
    


//' The reads in the specified region in an SRA record.
//'
//' This returns the all reads in the specified region.
//'
//' @param acc An accession or a path to an actual SRA file (with .sra suffix)
//' @param ref The reference name 
//' @param start Start position (inclusive)
//' @param stop End position (inclusive)
//' @return the reads in the collection
//' @export
//' @examples
//' getSRAReadsWithRegion('SRR789392','NC_000020.10', 62926240, 62958722)
// [[Rcpp::export]]
Rcpp::List getSRAReadsWithRegion(Rcpp::String acc, Rcpp::String refname, long start, long stop) {
  try {
    ReadCollection   run = ncbi::NGS::openReadCollection ( acc );
    
    //testing whether there is alignment
    try {
      long alignmentCount = run.getAlignmentCount();
      if (alignmentCount==0) {
        throw std::range_error("no aligned reads availabe"); 
        return(Rcpp::List());
      }
    } catch (ngs::ErrorMsg ngsErr){
      forward_exception_to_r(ngsErr);
      return(Rcpp::List());
    }
    
    try {
       if (!run.hasReference ( refname )) {
        std::string errorAndRefNames = "The accession id "+ string(acc) +" does not have the reference " +  
          string(refname) +  ". The options are:";
        ReferenceIterator refIter = run.getReferences();
        while( refIter.nextReference() ) {
          errorAndRefNames += " " + refIter.getCanonicalName();
        }
        throw std::range_error(errorAndRefNames); 
       }
    } catch (ngs::ErrorMsg ngsErr){
      forward_exception_to_r(ngsErr);
      return(Rcpp::List());
    }
    
    try {
      // get requested reference
      ngs::Reference ref = run.getReference ( refname );
      
      long referenceLength = ref.getLength();
      if (stop<start || stop>referenceLength || start<1) {
        throw std::range_error("wrong reference range, reference length = " + toString(referenceLength)); 
        return(Rcpp::List());
      } 
      
      long count = stop - start + 1;
      AlignmentIterator alignit = ref.getAlignmentSlice ( start, count, Alignment::primaryAlignment );
      vector<std::string> reads;
      vector<std::string> qualities;
      
      while( alignit.nextAlignment() ) {
        reads.push_back(alignit.getFragmentBases().toString());
      }
      return List::create (
          _["reads"] = reads
      );
      
    } catch (ngs::ErrorMsg ngsErr){
      forward_exception_to_r(ngsErr);
      return(Rcpp::List());
    }
  } catch(std::exception &ex) {	
    forward_exception_to_r(ex);
    return(Rcpp::List());
  } catch(...) { 
    ::Rf_error("c++ exception (unknown reason)"); 
    return(Rcpp::List());
  } //try ReadCollection run
  
}