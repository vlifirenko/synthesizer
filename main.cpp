#include <iostream>
using namespace std;

#include "olcNoiseMaker.h"

// converts freq to angular velocity
double w(double dHertz)
{
   return dHertz * 2.0 * PI;
}

double osc(double dHertz, double dTime, int nType)
{
   switch (nType)
   {
   case 0:  // square
      return sin(w(dHertz) * dTime);
   
   case 1:  // wave
      return sin(w(dHertz) * dTime) > 0.0 ? 1.0 : -1.0;

   case 2:  // triangle
      return asin(sin(w(dHertz) * dTime)) * (2.0 / PI);

   case 3:  // saw (analogue / warm / slow)
   {
      double dOutput = 0.0;

      for (double n = 1.0; n < 10.0; n++)
         dOutput += (sin(n * w(dHertz) * dTime)) / n;

      return dOutput * (2.0 / PI);
   }

   case 4:  // saw (optimised / harsh / fast)
      return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

   case 5:  // pseudo random noise
      return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

   default:
      return 0;
   }
}

atomic<double> dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 110.0;   // A2
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

double MakeNoise(double dTime)
{
   double dOutput = osc(dFrequencyOutput, dTime, 3);

   return dOutput * 0.4;
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