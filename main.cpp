#include <iostream>
using namespace std;

#include "olcNoiseMaker.h"

atomic<double> dFrequencyOutput = 0.0;

double MakeNoise(double dTime)
{
   double dOutput = 1.0 * sin(dFrequencyOutput * 2 * 3.14159 * dTime);

   if (dOutput > 0.0)
      return 0.2;
   else
      return -0.2;
}

int main()
{
   wcout << "Synthesizer" << endl;

   // get all sound hardware
   vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

   // display findings
   for (auto d : devices)
      wcout << "Found output device:" << d << endl;

   // create noise machine
   olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

   // link noise function with sound machine
   sound.SetUserFunction(MakeNoise);

   double dOctaveBaseFrequency = 110.0;   // A2
   double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

   while (1)
   {
      // keyboard

      bool bKeyPressed = false;
      for (int k = 0; k < 15; k++)
      {
         if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[k])) & 0x8000)
         {
            dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
            bKeyPressed = true;
         }
      }

      if (!bKeyPressed)
      {
         dFrequencyOutput = 0.0;
      }
   }

   return 0;
}