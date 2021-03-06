/*!
A sine wave with an amplitude and frequency:
\f[ \begin{array}{rcl} \varphi(t) & = & \int_0^t Frequency (t') \; dt' \\
GlobalEField & = & Amplitude \cdot \cos(2 \pi \varphi(t)) \end{array} \f]

\par Input/Output:
- GlobalEField: Generated sine-wave EOD in V

\par Parameter:
- Amplitude: Amplitude of sine-wave in V
- Frequency: Frequency of sine wave in Hz
*/

#if defined (__KERNEL__) || defined (DYNCLAMPMODEL)

  /*! Name, by which this model is known inside Linux: */
const char *modelName;

  /*! The period length of the realtime periodic task in seconds. */
float loopInterval;
  /*! One over the period length of the realtime periodic task in Hertz. */
float loopRate;

  /*! Analog input that is read from the DAQ board. */
#define INPUT_N 1
  /*! The \a inputNames are used to match the \a input variables with
      analog input traces in Relacs. */
const char *inputNames[INPUT_N] = { "EOD" };
const char *inputUnits[INPUT_N] = { "mV" };
  /*! The \a inputChannels are set automatically. */
int inputChannels[INPUT_N];
  /*! \a input holds the current value that was read in from the DAQ board. */
float input[INPUT_N] = { 0.0 };

  /*! Analog output that is written to the DAQ board. */
#define OUTPUT_N 1
const char *outputNames[OUTPUT_N] = { "GlobalEField" };
const char *outputUnits[OUTPUT_N] = { "V" };
int outputChannels[OUTPUT_N];
float output[OUTPUT_N] = { 0.0 };

  /*! Parameter that are provided by the model and can be read out. */
#define PARAMINPUT_N 0
const char *paramInputNames[PARAMINPUT_N];
const char *paramInputUnits[PARAMINPUT_N];
float paramInput[PARAMINPUT_N];

  /*! Parameter that are read by the model and are written to the model. */
#define PARAMOUTPUT_N 2
const char *paramOutputNames[PARAMOUTPUT_N] = { "Amplitude", "Frequency" };
const char *paramOutputUnits[PARAMOUTPUT_N] = { "V", "Hz" };
float paramOutput[PARAMOUTPUT_N] = { 0.0, 0.0 };

  /*! Variables used by the model. */
float phase = 0.0;

void initModel( void )
{
   modelName = "eod";
   phase = 0.0;
}

void computeModel( void )
{
#ifdef ENABLE_LOOKUPTABLES
  int k;
#endif

  // phase:
  phase += paramOutput[1] * loopInterval;
  if ( phase >= 1.0 )
    phase -= 1.0;
#ifdef ENABLE_LOOKUPTABLES
  // cosine from lookuptable:
  k = phase*lookupn[0];
  // eod:
  output[0] = paramOutput[0] * lookupy[0][k];
#else
  output[0] = paramOutput[0] * cos( 2.0*M_PI*phase );
#endif
}

#endif

#ifndef __KERNEL__
#ifdef ENABLE_LOOKUPTABLES

/*! This function is called from DynClampAnalogOutput in user
    space/C++ context and can be used to create some lookuptables for
    nonlinear functions to be used by computeModel(). The implementation of this
    functions has to allocate an \a x and \a y array of floats of a sensible size \a n.
    \param[in] \a k : the index for the lookup table to be generated.
    \param[out] \a n : the size of the lookup table (the number of elements in \a x and \a y).
    \param[out] \a x : the x-values.
    \param[out] \a y : the corresponding y-values.
    \return: 0 if a lookuptable was generated, -1 otherwise.
*/
int generateLookupTable( int k, float **x, float **y, int *n )
{
  if ( k == 0 ) {
    /* Lookup-table for one period of the sine function: */
    const int nn = 100000;
    *n = nn;
    *x = new float[nn];
    *y = new float[nn];
    for ( int j=0; j<nn; j++ ) {
      float xx = j*1.0/nn;
      (*x)[j] = xx;
      (*y)[j] = sin( 2.0*M_PI*xx );
    }
    return 0;
  }
  return -1;
}

#endif
#endif
