// Registo de todas as páginas, pela ordem do currículo.
// O "id" de cada página é o valor "web" dos exercícios em curriculum.json.

import type { PageDef } from "./types";
import { buffersPage } from "./buffers";
import { samplingPage } from "./sampling";
import { nyquistPage } from "./nyquist";
import { bitDepthPage } from "./bitDepth";
import { realtimePage } from "./realtime";
import { decibelsPage } from "./decibels";
import { panLawsPage } from "./panLaws";
import { smoothingPage } from "./smoothing";
import { delayLinePage } from "./delayLine";
import { fractionalDelayPage } from "./fractionalDelay";
import { feedbackDelayPage } from "./feedbackDelay";
import { onePolePage } from "./onePole";
import { biquadPage } from "./biquad";
import { svfPage } from "./svf";
import { lfoPage } from "./lfo";
import { chorusFlangerPage } from "./chorusFlanger";
import { waveshapingPage } from "./waveshaping";
import { bitcrushPage } from "./bitcrush";
import { fftPage } from "./fft";
import { oversamplingPage } from "./oversampling";
import { combAllpassPage } from "./combAllpass";
import { reverbPage } from "./reverb";
import { fdnPage } from "./fdn";
import { shimmerPage } from "./shimmer";

export const pages: PageDef[] = [
  buffersPage,
  samplingPage,
  nyquistPage,
  bitDepthPage,
  realtimePage,
  decibelsPage,
  panLawsPage,
  smoothingPage,
  delayLinePage,
  fractionalDelayPage,
  feedbackDelayPage,
  onePolePage,
  biquadPage,
  svfPage,
  lfoPage,
  chorusFlangerPage,
  waveshapingPage,
  bitcrushPage,
  fftPage,
  oversamplingPage,
  combAllpassPage,
  reverbPage,
  fdnPage,
  shimmerPage,
];
