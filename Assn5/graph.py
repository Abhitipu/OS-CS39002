import matplotlib.pyplot as plt
import os

if __name__ == "__main__":
    with open("output.txt", "r") as infile:
        with_gc = list(map(int, infile.readline().strip().split(" ")))
        print(with_gc)
        without_gc = list(map(int, infile.readline().strip().split(" ")))
        print(without_gc)
        plt.plot(with_gc, without_gc)
        plt.xlabel("With garbage collector")
        plt.ylabel("Without garbage collector")
        plt.savefig("Footprint.png")

