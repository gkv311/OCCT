out vec4 OutColor;

// Seed for random number generator (generated on CPU).
uniform int uFrameRndSeed;

//! Enables/disables using of single RNG seed for 16x16 image
//! blocks. Increases performance up to 4x, but the noise has
//! become structured. Can be used fo final rendering.
uniform int uBlockedRngEnabled;

#ifndef ADAPTIVE_SAMPLING
  //! Weight of current frame related to accumulated samples.
  uniform float uSampleWeight;

  //! Input accumulated image.
  uniform sampler2D uAccumTexture;
#endif

//! Maximum radiance that can be added to the pixel. Decreases noise
//! level, but introduces some bias.
#define MAX_RADIANCE vec3 (25.f)

// =======================================================================
// function : main
// purpose  :
// =======================================================================
void main (void)
{
  SeedRand (uFrameRndSeed, uWinSizeX, uBlockedRngEnabled == 0 ? 1 : 16);

#ifndef PATH_TRACING

  SRay aRay = GenerateRay (vPixel);

#else

  ivec2 aFragCoord = ivec2 (gl_FragCoord.xy);

#ifdef ADAPTIVE_SAMPLING

  ivec2 aTileXY = imageLoad (uOffsetImage, ivec2 (aFragCoord.x / BLOCK_SIZE,
                                                  aFragCoord.y / BLOCK_SIZE)).xy;

  ivec2 aRealBlockSize = ivec2 (min (uWinSizeX - aTileXY.x, BLOCK_SIZE),
                                min (uWinSizeY - aTileXY.y, BLOCK_SIZE));

  aFragCoord.x = aTileXY.x + (aFragCoord.x % aRealBlockSize.x);
  aFragCoord.y = aTileXY.y + (aFragCoord.y % aRealBlockSize.y);

#endif // ADAPTIVE_SAMPLING

  vec2 aPnt = vec2 (aFragCoord.x + RandFloat(),
                    aFragCoord.y + RandFloat());

  SRay aRay = GenerateRay (aPnt / vec2 (uWinSizeX, uWinSizeY));

#endif // PATH_TRACING

  vec3 aInvDirect = InverseDirection (aRay.Direct);

#ifdef PATH_TRACING

  vec4 aColor = PathTrace (aRay, aInvDirect);

  if (any (isnan (aColor.rgb)))
  {
    aColor.rgb = ZERO;
  }

  aColor.rgb = min (aColor.rgb, MAX_RADIANCE);

#ifdef ADAPTIVE_SAMPLING

  // accumulate RGB color and depth
  imageAtomicAdd (uRenderImage, ivec2 (3 * aFragCoord.x + 0,
                                       2 * aFragCoord.y + 0), aColor.r);
  imageAtomicAdd (uRenderImage, ivec2 (3 * aFragCoord.x + 1,
                                       2 * aFragCoord.y + 0), aColor.g);
  imageAtomicAdd (uRenderImage, ivec2 (3 * aFragCoord.x + 1,
                                       2 * aFragCoord.y + 1), aColor.b);
  imageAtomicAdd (uRenderImage, ivec2 (3 * aFragCoord.x + 2,
                                       2 * aFragCoord.y + 1), aColor.w);

  // accumulate number of samples
  float aNbSamples = imageAtomicAdd (uRenderImage, ivec2 (3 * aFragCoord.x + 0,
                                                          2 * aFragCoord.y + 1), 1.0);

  if (int (aNbSamples) % 2 == 0) // accumulate luminance for even samples only
  {
    imageAtomicAdd (uRenderImage, ivec2 (3 * aFragCoord.x + 2,
                                         2 * aFragCoord.y + 0), dot (LUMA, aColor.rgb));
  }

  discard; // fragment should not be written to frame buffer

#else

  if (uSampleWeight >= 1.f)
  {
    OutColor = aColor;
  }
  else
  {
    OutColor = mix (texture2D (uAccumTexture, vPixel), aColor, uSampleWeight);
  }

#endif // ADAPTIVE_SAMPLING

#else

  OutColor = clamp (Radiance (aRay, aInvDirect), 0.f, 1.f);

#endif // PATH_TRACING
}
