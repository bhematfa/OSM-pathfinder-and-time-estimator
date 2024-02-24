# OSM-pathfinder-and-time-estimator

This project implements an efficient pathfinding algorithm to calculate the shortest travel time between nodes on a map, using Dijkstra's algorithm optimized with a custom MinHeap data structure. It's particularly tailored for navigating complex maps, demonstrated using the University of Prince Edward Island's campus map as an example.

## Features

- **Efficient Pathfinding**: Utilizes Dijkstra's algorithm with a custom MinHeap for optimal performance.
- **Large Map Handling**: Capable of processing large maps with over 140,000 nodes and 14,000 ways.
- **Memory Management**: Implements dynamic memory management techniques to efficiently handle large datasets while preventing memory leaks and segmentation faults.
- **Scalable Design**: Designed for easy integration with additional datasets and extendability for new features.

## Technologies Used

- C
- Valgrind (for memory leak and error detection)
- MinHeap Data Structure
- Dijkstra's Algorithm

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

- GCC compiler
- Valgrind (for debugging)

### Installing

Clone the repository to your local machine:

```bash
git clone https://github.com/bhematfa/OSM-pathfinder-and-time-estimator.git
cd shortest-timed-path-finder

### Compiling using GCC

make

# Academic Integrity Reminder
If you are a student at the University of Toronto taking CSC209H, please remember that you are responsible for following the University's Academic Integrity Policy. You are reminded that copying any code from this repository without proper citation constitutes plagiarism and may result in an academic offense being raised against you. Should you find yourself in a situation where you are tempted to copy code from this repository, please take a step back and consider using course resources such as Office Hours or Piazza for assistance instead.

If you are not a student at the University of Toronto, please remember that you are responsible for following the Academic Integrity Policy of your institution. You are reminded that copying any code from this repository without proper citation may constitute plagiarism. You are encouraged to consult your institution's Academic Integrity Policy for further guidance.

Remember that copying code blindly is not only academically dishonest, but also counterproductive to your learning. If you are struggling with a concept, please reach out to your instructor or TA for help. They are there to help you, and will be more than happy to do so! This repository is intended to be used as a reference, not as a source of code to copy. Please use it responsibly.
