#include <iostream>
using namespace std;

#include "olcNoiseMaker.h"

// converts freq to angular velocity
double w(double dHertz)
{
   return dHertz * 2.0 * PI;
}

#define OSC_SINE 0
#define OSC_SQUARE 1
#define OSC_TRIANGLE 2
#define OSC_SAW_ANA 3
#define OSC_SAW_DIG 4
#define OSC_NOISE 5

double osc(double dHertz, double dTime, int nType = OSC_SINE, double dLFOHertz = 0.0, double dLFOAmplitude = 0.0)
{
   double dFreq = w(dHertz) * dTime + dLFOAmplitude * dHertz * sin(w(dLFOHertz) * dTime);

   switch (nType)
   {
   case OSC_SINE:  // sin
      return sin(dFreq);
   
   case OSC_SQUARE:  // square
      return sin(dFreq) > 0.0 ? 1.0 : -1.0;

   case OSC_TRIANGLE:  // triangle
      return asin(sin(dFreq) * (2.0 / PI));

   case OSC_SAW_ANA:  // saw (analogue / warm / slow)
   {
      double dOutput = 0.0;

      for (double n = 1.0; n < 10.0; n++)
         dOutput += (sin(n * dFreq)) / n;

      return dOutput * (2.0 / PI);
   }

   case OSC_SAW_DIG:  // saw (optimised / harsh / fast)
      return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

   case OSC_NOISE:  // pseudo random noise
      return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

   default:
      return 0;
   }
}

struct sEnvelopeADSR
{
   double dAttackTime;
   double dDecayTime;
   double dReleaseTime;

   double dSustainAmplitude;
   double dStartAmplitude;

   double dTriggerOnTime;
   double dTriggerOffTime;

   bool bNoteOn;

   sEnvelopeADSR()
   {
      dAttackTime = 0.01;
      dDecayTime = 1.0;
      dStartAmplitude = 1.0;
      dSustainAmplitude = 0.0;
      dReleaseTime = 1.0;
      dTriggerOnTime = 0.0;
      dTriggerOffTime = 0.0;
      bNoteOn = false;
   }

   double GetAmplitude(double dTime)
   {
      double dAmplitude = 0.0;
      double dLifeTime = dTime - dTriggerOnTime;

      if (bNoteOn)
      {
         // ADS

         // attack
         if (dLifeTime <= dAttackTime)
            dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

         // decay
         if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
            dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude)
               + dStartAmplitude;

         // sustain
         if (dLifeTime > (dAttackTime + dDecayTime))
         {
            dAmplitude = dSustainAmplitude;
         }

      }
      else
      {
         // release
         dAmplitude = ((dTime - dTriggerOffTime) / dReleaseTime) * (0.0 - dSustainAmplitude) + dSustainAmplitude;
      }

      if (dAmplitude < 0.0001)
         dAmplitude = 0;

      return dAmplitude;
   }

   void NoteOn(double dTimeOn)
   {
      dTriggerOnTime = dTimeOn;
      bNoteOn = true;
   }

   void NoteOff(double dTimeOff)
   {
      dTriggerOffTime = dTimeOff;
      bNoteOn = false;
   }
};

struct instrument
{
   double dVolume;
   sEnvelopeADSR env;

   virtual double sound(double dTime, double dFrequency) = 0;
};

struct bell : public instrument
{
   bell()
   {
      env.dAttackTime = 0.01;
      env.dDecayTime = 1.0;
      env.dStartAmplitude = 1.0;
      env.dSustainAmplitude = 0.0;
      env.dReleaseTime = 1.0;
      env.dTriggerOnTime = 0.0;
      env.dTriggerOffTime = 0.0;
   }

   double sound(double dTime, double dFrequency)
   {
      double dOutput = env.GetAmplitude(dTime) *
         (
            +1.0 * osc(dFrequency * 2.0, dTime, OSC_SINE, 5.0, 0.001)
            + 0.5 * osc(dFrequency * 3.0, dTime, OSC_SINE)
            + 0.25 * osc(dFrequency * 4.0, dTime, OSC_SINE)
          );

      return dOutput;
   }
};

struct harmonica : public instrument
{
   harmonica()
   {
      env.dAttackTime = 0.01;
      env.dDecayTime = 1.0;
      env.dStartAmplitude = 1.0;
      env.dSustainAmplitude = 0.0;
      env.dReleaseTime = 1.0;
      env.dTriggerOnTime = 0.0;
      env.dTriggerOffTime = 0.0;
   }

   double sound(double dTime, double dFrequency)
   {
      double dOutput = env.GetAmplitude(dTime) *
         (
            +1.0 * osc(dFrequency, dTime, OSC_SQUARE, 5.0, 0.001)
            + 0.5 * osc(dFrequency * 1.5, dTime, OSC_SQUARE)
            + 0.25 * osc(dFrequency * 2.0, dTime, OSC_SQUARE)
            + 0.05 * osc(0, dTime, OSC_NOISE)
            );

      return dOutput;
   }
};

atomic<double> dFrequencyOutput = 0.0;
double dOctaveBaseFrequency = 220.0;   // A3
double d12thRootOf2 = pow(2.0, 1.0 / 12.0);
sEnvelopeADSR envelope;

instrument *voice = nullptr;

double MakeNoise(double dTime)
{
   double dOutput = voice->sound(dTime, dFrequencyOutput);
   return dOutput * 0.4;
}

int main()
{
   wcout << "Synthesizer" << endl;

   // get all sound hardware
   vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

   // display findings
   //for (auto d : devices)
   //   wcout << "Found output device:" << d << endl;

   // create noise machine
   olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 512);

   //voice = new bell();
   voice = new harmonica();

   // link noise function with sound machine
   sound.SetUserFunction(MakeNoise);

   // keyboard
   int nCurrentKey = -1;
   bool bKeyPressed = false;

   while (1)
   {
      bKeyPressed = false;
      for (int k = 0; k < 15; k++)
      {
         if (GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe"[k])) & 0x8000)
         {
            if (nCurrentKey != k)
            {
               dFrequencyOutput = dOctaveBaseFrequency * pow(d12thRootOf2, k);
               voice->env.NoteOn(sound.GetTime());
               wcout << "\rNote On: " << sound.GetTime() << "s " << dFrequencyOutput << "Hz";
               nCurrentKey = k;
            }

            bKeyPressed = true;
         }
      }

      if (!bKeyPressed)
      {
         if (nCurrentKey != -1)
         {
            wcout << "\rNote Off: " << sound.GetTime() << "s                              ";
            voice->env.NoteOff(sound.GetTime());
            nCurrentKey = -1;
         }
      }
   }

   return 0;
}