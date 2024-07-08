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
   case 0:
      return sin(w(dHertz) * dTime);
   case 1:
      return sin(w(dHertz) * dTime) > 0.0 ? 1.0 : -1.0;

   default:
      return 0;
   }
}

atomic<double> dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 110.0;   // A2
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);

double MakeNoise(double dTime)
{
   double dOutput = 1.0 * (sin(w(dFrequencyOutput) * dTime));

   //+ sin((dFrequencyOutput + 20.0) * 2 * 3.14159 * dTime));

   return dOutput * 0.4;
   
   //if (dOutput > 0.0)
   //   return 0.2;
   //else
   //   return -0.2;
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