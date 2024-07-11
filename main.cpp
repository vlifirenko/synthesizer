#include <iostream>
using namespace std;

#define FTYPE double
#include "olcNoiseMaker.h"

namespace synth
{
   // converts freq to angular velocity
   FTYPE w(FTYPE dHertz)
   {
      return dHertz * 2.0 * PI;
   }

   struct instrument_base;

   // basic note
   struct note
   {
      int id;     // position in scale
      FTYPE on;
      FTYPE off;
      bool active;
      instrument_base *channel;

      note()
      {
         id = 0;
         on = 0.0;
         off = 0.0;
         active = false;
         channel = nullptr;
      }
   };

   const int OSC_SINE = 0;
   const int OSC_SQUARE = 1;
   const int OSC_TRIANGLE = 2;
   const int OSC_SAW_ANA = 3;
   const int OSC_SAW_DIG = 4;
   const int OSC_NOISE = 5;

   FTYPE osc(const FTYPE dHertz, const FTYPE dTime, const int nType = OSC_SINE,
      const FTYPE dLFOHertz = 0.0, const FTYPE dLFOAmplitude = 0.0, FTYPE dCustom = 50.0)
   {
      FTYPE dFreq = w(dHertz) * dTime + dLFOAmplitude * dHertz * (sin(w(dLFOHertz) * dTime));

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
         FTYPE dOutput = 0.0;

         for (FTYPE n = 1.0; n < 10.0; n++)
            dOutput += (sin(n * dFreq)) / n;

         return dOutput * (2.0 / PI);
      }

      case OSC_SAW_DIG:  // saw (optimised / harsh / fast)
         return (2.0 / PI) * (dHertz * PI * fmod(dTime, 1.0 / dHertz) - (PI / 2.0));

      case OSC_NOISE:  // pseudo random noise
         return 2.0 * ((double)rand() / (double)RAND_MAX) - 1.0;

      default:
         return 0.0;
      }
   }

   const int SCALE_DEFAULT = 0;

   FTYPE scale(const int nNoteID, const int nScaleID = SCALE_DEFAULT)
   {
      switch (nScaleID)
      {
      case SCALE_DEFAULT: default:
         return 256 * pow(1.0594630943592952645618252949463, nNoteID);
      }
   }

   struct envelope
   {
      virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff) = 0;
   };

   struct envelope_adsr : public envelope
   {
      FTYPE dAttackTime;
      FTYPE dDecayTime;
      FTYPE dReleaseTime;
      FTYPE dSustainAmplitude;
      FTYPE dStartAmplitude;

      envelope_adsr()
      {
         dAttackTime = 0.1;
         dDecayTime = 0.1;
         dSustainAmplitude = 1.0;
         dReleaseTime = 0.2;
         dStartAmplitude = 1.0;
      }

      virtual FTYPE amplitude(const FTYPE dTime, const FTYPE dTimeOn, const FTYPE dTimeOff)
      {
         FTYPE dAmplitude = 0.0;
         FTYPE dReleaseAmplitude = 0.0;

         if (dTimeOn > dTimeOff)   // note is on
         {
            FTYPE dLifeTime = dTime - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dAmplitude = dSustainAmplitude;

         }
         else  // note is off
         {
            FTYPE dLifeTime = dTimeOff - dTimeOn;

				if (dLifeTime <= dAttackTime)
					dReleaseAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;

				if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
					dReleaseAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;

				if (dLifeTime > (dAttackTime + dDecayTime))
					dReleaseAmplitude = dSustainAmplitude;

				dAmplitude = ((dTime - dTimeOff) / dReleaseTime) * (0.0 - dReleaseAmplitude) + dReleaseAmplitude;
         }

         if (dAmplitude <= 0.000)
            dAmplitude = 0.0;

         return dAmplitude;
      }
   };

   FTYPE env(const FTYPE dTime, envelope& env, const FTYPE dTimeOn, const FTYPE dTimeOff)
   {
      return env.amplitude(dTime, dTimeOn, dTimeOff);
   }

   struct instrument_base
   {
      FTYPE dVolume;
      synth::envelope_adsr env;
      FTYPE fMaxLifeTime;
      wstring name;
      virtual FTYPE sound(const FTYPE dTime, synth::note n, bool& bNoteFinished) = 0;
   };

   struct instrument_bell : public instrument_base
   {
      instrument_bell()
      {
         env.dAttackTime = 0.01;
         env.dDecayTime = 1.0;
         env.dSustainAmplitude = 0.0;
         env.dReleaseTime = 1.0;
         fMaxLifeTime = 3.0;
         dVolume = 1.0;
         name = L"Bell";
      }

      virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
      {
         FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
         if (dAmplitude <= 0.0)
            bNoteFinished = true;

         FTYPE dSound =
            +1.00 * synth::osc(n.on - dTime, synth::scale(n.id + 12), synth::OSC_SINE, 5.0, 0.001)
            + 0.50 * synth::osc(n.on - dTime, synth::scale(n.id + 24))
            + 0.25 * synth::osc(n.on - dTime, synth::scale(n.id + 36));

         return dAmplitude * dSound * dVolume;
      }
   };

   struct instrument_bell8 : public instrument_base
   {
      instrument_bell8()
      {
         env.dAttackTime = 0.01;
         env.dDecayTime = 0.5;
         env.dSustainAmplitude = 0.8;
         env.dReleaseTime = 1.0;
         fMaxLifeTime = 3.0;
         dVolume = 1.0;
         name = L"8-Bit Bell";
      }

      virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
      {
         FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
         if (dAmplitude <= 0.0) bNoteFinished = true;

         FTYPE dSound =
            +1.00 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
            + 0.50 * synth::osc(n.on - dTime, synth::scale(n.id + 12))
            + 0.25 * synth::osc(n.on - dTime, synth::scale(n.id + 24));

         return dAmplitude * dSound * dVolume;
      }

   };

   struct instrument_harmonica : public instrument_base
   {
      instrument_harmonica()
      {
         env.dAttackTime = 0.00;
         env.dDecayTime = 1.0;
         env.dSustainAmplitude = 0.95;
         env.dReleaseTime = 0.1;
         fMaxLifeTime = -1.0;
         name = L"Harmonica";
         dVolume = 0.3;
      }

      virtual FTYPE sound(const FTYPE dTime, synth::note n, bool &bNoteFinished)
      {
         FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
         if (dAmplitude <= 0.0) bNoteFinished = true;

         FTYPE dSound =
            //+ 1.0  * synth::osc(n.on - dTime, synth::scale(n.id-12), synth::OSC_SAW_ANA, 5.0, 0.001, 100)
            +1.00 * synth::osc(n.on - dTime, synth::scale(n.id), synth::OSC_SQUARE, 5.0, 0.001)
            + 0.50 * synth::osc(n.on - dTime, synth::scale(n.id + 12), synth::OSC_SQUARE)
            + 0.05 * synth::osc(n.on - dTime, synth::scale(n.id + 24), synth::OSC_NOISE);

         return dAmplitude * dSound * dVolume;
      }

   };

   struct instrument_drumkick : public instrument_base
   {
      instrument_drumkick()
      {
         env.dAttackTime = 0.01;
         env.dDecayTime = 0.15;
         env.dSustainAmplitude = 0.0;
         env.dReleaseTime = 0.0;
         fMaxLifeTime = 1.5;
         name = L"Drum Kick";
         dVolume = 1.0;
      }

      virtual FTYPE sound(const FTYPE dTime, synth::note n, bool& bNoteFinished)
      {
         FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
         if (fMaxLifeTime > 0.0 && dTime - n.on >= fMaxLifeTime)	bNoteFinished = true;

         FTYPE dSound =
            +0.99 * synth::osc(dTime - n.on, synth::scale(n.id - 36), synth::OSC_SINE, 1.0, 1.0)
            + 0.01 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

         return dAmplitude * dSound * dVolume;
      }

   };

   struct instrument_drumsnare : public instrument_base
   {
      instrument_drumsnare()
      {
         env.dAttackTime = 0.0;
         env.dDecayTime = 0.2;
         env.dSustainAmplitude = 0.0;
         env.dReleaseTime = 0.0;
         fMaxLifeTime = 1.0;
         name = L"Drum Snare";
         dVolume = 1.0;
      }

      virtual FTYPE sound(const FTYPE dTime, synth::note n, bool& bNoteFinished)
      {
         FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
         if (fMaxLifeTime > 0.0 && dTime - n.on >= fMaxLifeTime)	bNoteFinished = true;

         FTYPE dSound =
            +0.5 * synth::osc(dTime - n.on, synth::scale(n.id - 24), synth::OSC_SINE, 0.5, 1.0)
            + 0.5 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

         return dAmplitude * dSound * dVolume;
      }

   };


   struct instrument_drumhihat : public instrument_base
   {
      instrument_drumhihat()
      {
         env.dAttackTime = 0.01;
         env.dDecayTime = 0.05;
         env.dSustainAmplitude = 0.0;
         env.dReleaseTime = 0.0;
         fMaxLifeTime = 1.0;
         name = L"Drum HiHat";
         dVolume = 0.5;
      }

      virtual FTYPE sound(const FTYPE dTime, synth::note n, bool& bNoteFinished)
      {
         FTYPE dAmplitude = synth::env(dTime, env, n.on, n.off);
         if (fMaxLifeTime > 0.0 && dTime - n.on >= fMaxLifeTime)	bNoteFinished = true;

         FTYPE dSound =
            +0.1 * synth::osc(dTime - n.on, synth::scale(n.id - 12), synth::OSC_SQUARE, 1.5, 1)
            + 0.9 * synth::osc(dTime - n.on, 0, synth::OSC_NOISE);

         return dAmplitude * dSound * dVolume;
      }

   };

   struct sequencer
   {
   public:
      struct channel
      {
         instrument_base* instrument;
         wstring sBeat;
      };

   public:
      sequencer(float tempo = 120.0f, int beats = 4, int subbeats = 4)
      {
         nBeats = beats;
         nSubBeats = subbeats;
         fTempo = tempo;
         fBeatTime = (60.0f / fTempo) / (float)nSubBeats;
         nCurrentBeat = 0;
         nTotalBeats = nSubBeats * nBeats;
         fAccumulate = 0;
      }

      int Update(FTYPE fElapsedTime)
      {
         vecNotes.clear();

         fAccumulate += fElapsedTime;
         while (fAccumulate >= fBeatTime)
         {
            fAccumulate -= fBeatTime;
            nCurrentBeat++;

            if (nCurrentBeat >= nTotalBeats)
               nCurrentBeat = 0;

            int c = 0;
            for (auto v : vecChannel)
            {
               if (v.sBeat[nCurrentBeat] == L'X')
               {
                  note n;
                  n.channel = vecChannel[c].instrument;
                  n.active = true;
                  n.id = 64;
                  vecNotes.push_back(n);
               }
               c++;
            }
         }

         return vecNotes.size();
      }

      void AddInstrument(instrument_base* inst)
      {
         channel c;
         c.instrument = inst;
         vecChannel.push_back(c);
      }

   public:
      int nBeats;
      int nSubBeats;
      FTYPE fTempo;
      FTYPE fBeatTime;
      FTYPE fAccumulate;
      int nCurrentBeat;
      int nTotalBeats;

   public:
      vector<channel> vecChannel;
      vector<note> vecNotes;
   };
}

vector<synth::note> vecNotes;
mutex muxNotes;
synth::instrument_bell instBell;
synth::instrument_harmonica instHarm;
synth::instrument_drumkick instKick;
synth::instrument_drumsnare instSnare;
synth::instrument_drumhihat instHiHat;

typedef bool(*lambda)(synth::note const& item);
template<class T>
void safe_remove(T& v, lambda f)
{
   auto n = v.begin();
   while (n != v.end())
   {
      if (!f(*n))
         n = v.erase(n);
      else
         ++n;
   }
}


FTYPE MakeNoise(int nChannel, double dTime)
{
   unique_lock<mutex> lm(muxNotes);
   FTYPE dMixedOutput = 0.0;

   for (auto &n : vecNotes)
   {
      bool bNoteFinished = false;
      FTYPE dSound = 0;

      if (n.channel != nullptr)
         dSound = n.channel->sound(dTime, n, bNoteFinished);

      dMixedOutput += dSound;

      if (bNoteFinished && n.off > n.on)
      {
         n.active = false;
      }
   }

   safe_remove<vector<synth::note>>(vecNotes,
      [](synth::note const& item) { return item.active; });

   return dMixedOutput * 0.2;
}

int main()
{
   vector<wstring> devices = olcNoiseMaker<short>::Enumerate();

   olcNoiseMaker<short> sound(devices[0], 44100, 1, 8, 256);

   sound.SetUserFunction(MakeNoise);

   wchar_t* screen = new wchar_t[80 * 30];
   HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
   SetConsoleActiveScreenBuffer(hConsole);
   DWORD dwBytesWritten = 0;

   auto draw = [&screen](int x, int y, wstring s)
      {
         for (size_t i = 0; i < s.size(); i++)
            screen[y * 80 + x + i] = s[i];
      };

   auto clock_old_time = chrono::high_resolution_clock::now();
   auto clock_real_time = chrono::high_resolution_clock::now();
   double dElapsedTime = 0.0;
   double dWallTime = 0.0;

   synth::sequencer seq(90.0);
   seq.AddInstrument(&instKick);
   seq.AddInstrument(&instSnare);
   seq.AddInstrument(&instHiHat);

   seq.vecChannel.at(0).sBeat = L"X...X...X..X.X..";
   seq.vecChannel.at(1).sBeat = L"..X...X...X...X.";
   seq.vecChannel.at(2).sBeat = L"X.X.X.X.X.X.X.XX";

   while (1)
   {
      clock_real_time = chrono::high_resolution_clock::now();
      auto time_last_loop = clock_real_time - clock_old_time;
      clock_old_time = clock_real_time;
      dElapsedTime = chrono::duration<FTYPE>(time_last_loop).count();
      dWallTime += dElapsedTime;
      FTYPE dTimeNow = sound.GetTime();

      int newNotes = seq.Update(dElapsedTime);
      muxNotes.lock();
      for (int a = 0; a < newNotes; a++)
      {
         seq.vecNotes[a].on = dTimeNow;
         vecNotes.emplace_back(seq.vecNotes[a]);
      }
      muxNotes.unlock();

      for (int k = 0; k < 16; k++)
      {
         short nKeyState = GetAsyncKeyState((unsigned char)("ZSXCFVGBNJMK\xbcL\xbe\xbf"[k]));

         muxNotes.lock();
         auto noteFound = find_if(vecNotes.begin(), vecNotes.end(), [&k](synth::note const& item) { return item.id == k + 64 && item.channel == &instHarm; });
         if (noteFound == vecNotes.end())
         {
            if (nKeyState & 0x8000)
            {
               synth::note n;
               n.id = k + 64;
               n.on = dTimeNow;
               n.active = true;
               n.channel = &instHarm;

               vecNotes.emplace_back(n);
            }
         }
         else
         {
            if (nKeyState & 0x8000)
            {
               if (noteFound->off > noteFound->on)
               {
                  noteFound->on = dTimeNow;
                  noteFound->active = true;
               }
            }
            else
            {
               if (noteFound->off < noteFound->on)
                  noteFound->off = dTimeNow;
            }
         }
         muxNotes.unlock();
      }

      for (int i = 0; i < 80 * 30; i++) screen[i] = L' ';

      draw(2, 2, L"SEQUENCER:");
      for (int beats = 0; beats < seq.nBeats; beats++)
      {
         draw(beats * seq.nSubBeats + 20, 2, L"O");
         for (int subbeats = 1; subbeats < seq.nSubBeats; subbeats++)
            draw(beats * seq.nSubBeats + subbeats + 20, 2, L".");
      }

      int n = 0;
      for (auto v : seq.vecChannel)
      {
         draw(2, 3 + n, v.instrument->name);
         draw(20, 3 + n, v.sBeat);
         n++;
      }

      draw(20 + seq.nCurrentBeat, 1, L"|");

      draw(2, 8, L"|   |   |   |   |   | |   |   |   |   | |   | |   |   |   |  ");
      draw(2, 9, L"|   | S |   |   | F | | G |   |   | J | | K | | L |   |   |  ");
      draw(2, 10, L"|   |___|   |   |___| |___|   |   |___| |___| |___|   |   |__");
      draw(2, 11, L"|     |     |     |     |     |     |     |     |     |     |");
      draw(2, 12, L"|  Z  |  X  |  C  |  V  |  B  |  N  |  M  |  ,  |  .  |  /  |");
      draw(2, 13, L"|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|");

      wstring stats = L"Notes: " + to_wstring(vecNotes.size()) + L" Wall Time: " + to_wstring(dWallTime) + L" CPU Time: " + to_wstring(dTimeNow) + L" Latency: " + to_wstring(dWallTime - dTimeNow);
      draw(2, 15, stats);

      WriteConsoleOutputCharacter(hConsole, screen, 80 * 30, { 0,0 }, &dwBytesWritten);
   }


   return 0;
}