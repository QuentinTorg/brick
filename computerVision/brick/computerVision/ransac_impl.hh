/**
***************************************************************************
* @file dlrComputerVision/ransac_impl.hh
*
* Header file defining helper functions for implementing Fischler's
* and Bolles's RANSAC algorithm.
*
* Copyright (C) 2008, 2011 David LaRose, dlr@cs.cmu.edu
* See accompanying file, LICENSE.TXT, for details.
***************************************************************************
*/

#ifndef BRICK_COMPUTERVISION_RANSAC_IMPL_HH
#define BRICK_COMPUTERVISION_RANSAC_IMPL_HH

// This file is included by ransac.hh, and should not be directly included
// by user code, so no need to include ransac.hh here.
//
// #include <brick/computerVision/ransac.hh>

#include <cmath>
#include <brick/common/exception.hh>


namespace brick {

  namespace computerVision {

    // Selects only those elements of the input sequence that, when
    // passed as arguments to functor.operator()(), result in a true
    // return value.
    template <class InIter, class OutIter, class Functor>
    unsigned int
    ransacGetConsensusSet(
      InIter inBegin, InIter inEnd, OutIter outBegin, Functor functor)
    {
      unsigned int count = 0;
      while(inBegin != inEnd) {
        if(functor(*inBegin)) {
          *outBegin = *inBegin;
          ++outBegin;
          ++count;
        }
        ++inBegin;
      }
      return count;
    }


    // This is a convenience function that functions just like
    // ransacGetConsensusSet, except that the output of the functor
    // argument is passed to a second functor for evaluation.
    template <class InIter, class OutIter, class Functor, class Criterion>
    unsigned int
    ransacGetConsensusSetByComparison(
      InIter inBegin, InIter inEnd, OutIter outBegin, Functor functor,
      Criterion criterion)
    {
      unsigned int count = 0;
      while(inBegin != inEnd) {
        if(criterion(functor(*inBegin))) {
          *outBegin = *inBegin;
          ++outBegin;
          ++count;
        }
        ++inBegin;
      }
      return count;
    }


    // This functions just like ransacGetConsensusSet, except that the
    // input and output sequences are replaced by 2D arrays.
    template <class Type, class Functor>
    brick::numeric::Array2D<Type>
    ransacGetConsensusSetRows(
      brick::numeric::Array2D<Type> const& candidates,
      Functor functor)
    {
      brick::numeric::Array1D<bool> indicatorArray(candidates.rows());
      unsigned int count = 0;
      for(unsigned int ii = 0; ii < candidates.rows(); ++ii) {
        brick::numeric::Array1D<Type> currentRow = candidates.getRow(ii);
        if(functor(currentRow)) {
          indicatorArray[ii] = true;
          ++count;
        } else {
          indicatorArray[ii] = false;
        }
      }

      brick::numeric::Array2D<Type> result(count, candidates.columns());
      unsigned int outputRow = 0;
      for(unsigned int ii = 0; ii < candidates.rows(); ++ii) {
        if(indicatorArray[ii]) {
          result.getRow(outputRow).copy(candidates.getRow(ii));
          ++outputRow;
        }
      }
      return result;
    }


    // This is is a convenience function that functions just like
    // ransacGetConsensusSetRows, except that the output of the
    // functor argument is passed to a second functor for evaluation.
    template <class Type, class Functor, class Criterion>
    brick::numeric::Array2D<Type>
    ransacGetConsensusSetRowsByComparison(
      brick::numeric::Array2D<Type> const& candidates,
      Functor functor,
      Criterion criterion)
    {
      // TBD: implement this by composing criterion and functor, then
      // calling ransacGetConsensusSet().
      brick::numeric::Array1D<bool> indicatorArray(candidates.rows());
      unsigned int count = 0;
      for(unsigned int ii = 0; ii < candidates.rows(); ++ii) {
        brick::numeric::Array1D<Type> currentRow = candidates.getRow(ii);
        if(criterion(functor(currentRow))) {
          indicatorArray[ii] = true;
          ++count;
        } else {
          indicatorArray[ii] = false;
        }
      }

      brick::numeric::Array2D<Type> result(count, candidates.columns());
      unsigned int outputRow = 0;
      for(unsigned int ii = 0; ii < candidates.rows(); ++ii) {
        if(indicatorArray[ii]) {
          result.getRow(outputRow).copy(candidates.getRow(ii));
          ++outputRow;
        }
      }
      return result;
    }



    // Randomly (or rather, pseudo-randomly) selects elements of the
    // input sequence for use in RANSAC estimation.
    template <class Type>
    std::vector<Type>
    ransacSelectElements(std::vector<Type> const& inputSequence,
                         unsigned int numberOfSamplesRequired)
    {
      brick::random::PseudoRandom pseudoRandom;
      std::vector<Type> result(numberOfSamplesRequired);
      std::vector<Type> shuffleBuffer(inputSequence.size());
      std::vector<bool> indicators(inputSequence.size(), false);

      // Select each sample in turn.  We will sample without replacement.
      for(unsigned int ii = 0; ii < numberOfSamplesRequired; ++ii) {
        // Easy enough: choose from among the remaining samples.
        unsigned int selectedIndex = pseudoRandom.uniformInt(
          ii, inputSequence.size());

        // Copy from the input sequence, unless this row has already been
        // selected.
        if(indicators[selectedIndex]) {
          // This row _has_ already been selected.  Fortunately, last
          // time it was selected, we had the forsight to copy an
          // un-selected row into shufflebuffer.  Use that one
          // instead.
          result[ii] = shuffleBuffer[selectedIndex];
        } else {
          // This row _has not_ already been selected.  Carry on.
          result[ii] = inputSequence[selectedIndex];
        }

        // The row that was just selected might get selected again
        // later, so we remove it from circulation by copying a
        // not-selected row into its place in shuffleBuffer.  We do
        // this unless selected == ii, in which case the lower
        // bound of the uniformInt() call above prevents
        // reselection of the row, so we do nothing.
        if(selectedIndex != ii) {
          shuffleBuffer[selectedIndex] = inputSequence[ii];
          indicators[selectedIndex] = true;
        }
      }
      return result;
    }


    template <class Type>
    brick::numeric::Array2D<Type>
    ransacSelectRows(brick::numeric::Array2D<Type> const& sampleArray,
                     unsigned int numberOfSamplesRequired)
    {
      brick::random::PseudoRandom pseudoRandom;
      return ransacSelectRows(sampleArray, numberOfSamplesRequired,
                              pseudoRandom);
    }


    // This function is just like ransacSelectRows, except that a
    // third argument allows the calling context to control the
    // pseudo-random sequence.
    template <class Type>
    brick::numeric::Array2D<Type>
    ransacSelectRows(brick::numeric::Array2D<Type> const& sampleArray,
                     unsigned int numberOfSamplesRequired,
                     brick::common::Int64& seed)
    {
      brick::random::PseudoRandom pseudoRandom(seed);
      brick::numeric::Array1D<Type> result =
        ransacSelectRows(sampleArray, numberOfSamplesRequired, pseudoRandom);
      seed = pseudoRandom.getCurrentSeed();
      return result;
    }


    // This function is just like ransacSelectRows, except that a
    // third argument allows the calling context to control the
    // pseudo-random sequence.
    template <class Type>
    brick::numeric::Array2D<Type>
    ransacSelectRows(brick::numeric::Array2D<Type> const& sampleArray,
                     unsigned int numberOfSamplesRequired,
                     brick::random::PseudoRandom& pseudoRandom)
    {
      brick::numeric::Array2D<Type> result(numberOfSamplesRequired,
                                           sampleArray.columns());
      brick::numeric::Array2D<Type> shuffleBuffer(sampleArray.rows(),
                                                  sampleArray.columns());
      brick::numeric::Array1D<bool> indicators(sampleArray.rows());
      indicators = false;

      // Select each sample in turn.  We will sample without replacement.
      for(unsigned int ii = 0; ii < numberOfSamplesRequired; ++ii) {
        // Easy enough: choose from among the remaining samples.
        unsigned int selectedRowIndex = pseudoRandom.uniformInt(
          ii, sampleArray.rows());
        // Copy from the input array, unless this row has already been
        // selected.
        if(indicators[selectedRowIndex]) {
          // This row _has_ already been selected.  Fortunately, last
          // time it was selected, we had the forsight to copy an
          // un-selected row into shufflebuffer.  Use that one
          // instead.
          result.getRow(ii).copy(shuffleBuffer.getRow(selectedRowIndex));
        } else {
          // This row _has not_ already been selected.  Carry on.
          result.getRow(ii).copy(sampleArray.getRow(selectedRowIndex));
        }

        // The row that was just selected might get selected again
        // later, so we remove it from circulation by copying a
        // not-selected row into its place in shuffleBuffer.  We do
        // this unless selectedRow == ii, in which case the lower
        // bound of the uniformInt() call above prevents
        // reselection of the row, so we do nothing.
        if(selectedRowIndex != ii) {
          shuffleBuffer.getRow(selectedRowIndex).copy(sampleArray.getRow(ii));
          indicators[selectedRowIndex] = true;
        }
      }
      return result;
    }

  } // namespace computerVision

} // namespace brick

#endif /* #ifndef BRICK_COMPUTERVISION_RANSAC_IMPL_HH */
