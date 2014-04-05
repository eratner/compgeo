//
//  main.cpp
//
//  Created by John Visentin on 4/2/14.
//  Copyright (c) 2014 John Visentin. All rights reserved.
//

#include "MPHeap.h"
#include "MPEnvironment2D.h"
#include <iostream>
#include <cmath>
#include <cstdlib>

void test2D();

int main(int argc, char** argv)
{
  test2D();

  return 0;
}

void test2D()
{
  // Initialize the data
  std::vector<MP::HeapElement<MP::Point2D> > data;
  for(int i = 0; i < 10; ++i)
  {
    MP::HeapElement<MP::Point2D> h;
    int k = rand() % 100 + 1;
    //int k = i+1;
    // Distance from the origin
    h.key = std::sqrt(2*k*k);
    h.state = new MP::SearchState2D();
    h.state->setValue(MP::Point2D(k, k));
    std::cout << "Adding state (" << h.state->getValue().x_ << ", " 
	      << h.state->getValue().y_ << ")" << std::endl;
    data.push_back(h);
  }

  // Make the heap
  MP::Heap<MP::Point2D> H;
  H.buildHeap(data);

  H.print();

  H.decreaseKey(data[3].state, 0.5);

  H.print();

  // Print the states in order of their distance from the origin
  int N = H.size();
  for(int i = 0; i < N; ++i)
  {
    MP::HeapElement<MP::Point2D> h = H.remove();
    std::cout << h.state->getHeapIndex() << ": (" << h.state->getValue().x_ << ", " 
	      << h.state->getValue().y_ << ")" << std::endl;
  }
}

