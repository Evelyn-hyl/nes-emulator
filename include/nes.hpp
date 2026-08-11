#ifndef NES_EMULATOR_HPP
#define NES_EMULATOR_HPP

#include "./cpu.hpp"
#include <string>

class NES {
  private:
    CPU cpu();

  public:

    /*
The ROM Loader (The Delivery Truck):Runs only once when you open a game file.Reads the file from your computer disk.Checks the file header to see which mapper the game needs.Delivers the game data into your computer's RAM.Closes the file and stops working.
The Mapper (The Bookshelf):Runs constantly while you are playing the game.Acts as the middleman between the CPU and the game data.Swaps different pages of game data in and out of the CPU's view.Stays active until you turn the game off.
    */
    
    void load_rom(std::string path);
};

#endif
