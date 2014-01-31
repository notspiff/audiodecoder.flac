/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "xbmc/libXBMC_addon.h"

extern "C" {
#include <FLAC/stream_decoder.h>
#include <FLAC/metadata.h>
#include "xbmc/xbmc_audiodec_dll.h"
#include "xbmc/AEChannelData.h"
#include <inttypes.h>

ADDON::CHelper_libXBMC_addon *XBMC           = NULL;

bool registerHelper(void* hdl)
{
  if (!XBMC)
    XBMC = new ADDON::CHelper_libXBMC_addon;

  if (!XBMC->RegisterMe(hdl))
  {
    delete XBMC, XBMC=NULL;
    return false;
  }

  return true;
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!registerHelper(hdl))
    return ADDON_STATUS_PERMANENT_FAILURE;

  return ADDON_STATUS_OK;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Destroy()
{
  XBMC=NULL;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
bool ADDON_HasSettings()
{
  return false;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}

struct FLACContext
{
  void* file;
  FLAC__StreamDecoder* decoder;
  uint8_t* buffer;
  size_t buffersize;
  const enum AEChannel* cinfo;
  int samplerate;
  int channels;
  int bitspersample;
  int64_t totaltime;
  size_t framesize;
  AEDataFormat format;

  FLACContext()
  {
    file = NULL;
    decoder = NULL;
    buffer = NULL;
    cinfo = NULL;
  }

  ~FLACContext()
  {
    if (decoder)
    {
      FLAC__stream_decoder_finish(decoder);
      FLAC__stream_decoder_delete(decoder);
    }
    if (file)
      XBMC->CloseFile(file);
    delete[] buffer;
  }
};

FLAC__StreamDecoderReadStatus DecoderReadCallback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
  FLACContext* pThis=(FLACContext*)client_data;
  if (!pThis || !XBMC)
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

  *bytes=XBMC->ReadFile(pThis->file, buffer, *bytes);
  if (*bytes == 0)
    return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;

  return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus DecoderSeekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
  FLACContext* pThis=(FLACContext*)client_data;
  if (!pThis || !XBMC)
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;

  if (XBMC->SeekFile(pThis->file,absolute_byte_offset, SEEK_SET)<0)
    return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;

  return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus DecoderTellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
  FLACContext* pThis=(FLACContext*)client_data;
  if (!pThis || !XBMC)
    return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

  *absolute_byte_offset=XBMC->GetFilePosition(pThis->file);

  return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus DecoderLengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
  FLACContext* pThis=(FLACContext*)client_data;
  if (!pThis || !XBMC)
    return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;

  *stream_length=XBMC->GetFileLength(pThis->file);

  return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool DecoderEofCallback(const FLAC__StreamDecoder *decoder, void *client_data)
{
  FLACContext* pThis=(FLACContext*)client_data;
  if (!pThis || !XBMC)
    return true;

  return XBMC->GetFileLength(pThis->file)==XBMC->GetFilePosition(pThis->file);
}

FLAC__StreamDecoderWriteStatus DecoderWriteCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
  FLACContext* pThis=(FLACContext*)client_data;
  if (!pThis || !XBMC)
    return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

  const int bytes_per_sample = frame->header.bits_per_sample/8;
  uint8_t* outptr = pThis->buffer+pThis->buffersize;
  FLAC__int16* outptr16 = (FLAC__int16 *) outptr;
  FLAC__int32* outptr32 = (FLAC__int32 *) outptr;

  unsigned int current_sample = 0;
  for(current_sample = 0; current_sample < frame->header.blocksize; current_sample++)
  {
    for(unsigned int channel = 0; channel < frame->header.channels; channel++)
    {
      switch(bytes_per_sample)
      {
        case 2:
          outptr16[current_sample*frame->header.channels + channel] = (FLAC__int16) buffer[channel][current_sample];
          break;
        case 3:
          outptr[2] = (buffer[channel][current_sample] >> 16) & 0xff;
          outptr[1] = (buffer[channel][current_sample] >> 8 ) & 0xff;
          outptr[0] = (buffer[channel][current_sample] >> 0 ) & 0xff;
          outptr += bytes_per_sample;
          break;
        default:
          outptr32[current_sample*frame->header.channels + channel] = buffer[channel][current_sample];
          break;
      }
    }
  }

  if (bytes_per_sample == 1)
  {
    for(unsigned int i=0;i<current_sample;i++)
    {
      uint8_t* outptr=pThis->buffer+pThis->buffersize;
      outptr[i]^=0x80;
    }
  }

  pThis->buffersize += current_sample*bytes_per_sample*frame->header.channels;

  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void DecoderMetadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
  FLACContext* pThis=(FLACContext*)client_data;
  if (!pThis)
    return;

  if (metadata->type==FLAC__METADATA_TYPE_STREAMINFO)
  {
    static enum AEChannel map[8][9] = {
      {AE_CH_FC, AE_CH_NULL},
      {AE_CH_FL, AE_CH_FR, AE_CH_NULL},
      {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_NULL},
      {AE_CH_FL, AE_CH_FR, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
      {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_BL, AE_CH_BR, AE_CH_NULL},
      {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_NULL}, // 6 channels
      {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BC, AE_CH_BL, AE_CH_BR, AE_CH_NULL}, // 7 channels
      {AE_CH_FL, AE_CH_FR, AE_CH_FC, AE_CH_LFE, AE_CH_BL, AE_CH_BR, AE_CH_SL, AE_CH_SR, AE_CH_NULL} // 8 channels
    };

    /* channel counts greater then 6 are undefined */
    if (metadata->data.stream_info.channels <= 8)
      pThis->cinfo = map[metadata->data.stream_info.channels - 1];

    pThis->samplerate = metadata->data.stream_info.sample_rate;
    pThis->channels = metadata->data.stream_info.channels;
    pThis->bitspersample = metadata->data.stream_info.bits_per_sample;
    switch(pThis->bitspersample)
    {
      case  8: pThis->format = AE_FMT_U8;     break;
      case 16: pThis->format = AE_FMT_S16NE;  break;
      case 24: pThis->format = AE_FMT_S24NE3; break;
      case 32: pThis->format = AE_FMT_FLOAT;  break;
    }
    pThis->totaltime = (int64_t)metadata->data.stream_info.total_samples * 1000 / metadata->data.stream_info.sample_rate;
    pThis->framesize = metadata->data.stream_info.max_blocksize*(pThis->bitspersample/8)*(pThis->channels);
  }
}

void DecoderErrorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
  if (!XBMC)
    return;

  XBMC->Log(ADDON::LOG_ERROR, "FLACCodec: Read error %i", status);
}

void* Init(const char* strFile, unsigned int filecache, int* channels,
           int* samplerate, int* bitspersample, int64_t* totaltime,
           int* bitrate, AEDataFormat* format, const AEChannel** channelinfo)
{
  if (!XBMC)
    return NULL;

  FLACContext* result = new FLACContext;

  result->decoder = FLAC__stream_decoder_new();
  if (!result->decoder)
  {
    delete result;
    return NULL;
  }

  result->file = XBMC->OpenFile(strFile,0);
  if (!result->file)
  {
    delete result;
    return NULL;
  }

  if (FLAC__stream_decoder_init_stream(result->decoder, DecoderReadCallback,
                                                        DecoderSeekCallback,
                                                        DecoderTellCallback,
                                                        DecoderLengthCallback,
                                                        DecoderEofCallback,
                                                        DecoderWriteCallback,
                                                        DecoderMetadataCallback,
                                                        DecoderErrorCallback,
                                                        result) 
      != FLAC__STREAM_DECODER_INIT_STATUS_OK)
  {
    delete result;
    return NULL;
  }

  if (!FLAC__stream_decoder_process_until_end_of_metadata(result->decoder))
  {
    delete result;
    return NULL;
  }

  *channels      = result->channels;
  *samplerate    = result->samplerate;
  *bitspersample = result->bitspersample;
  *totaltime     = result->totaltime;
  *format        = result->format;
  *channelinfo   = result->cinfo;
  *bitrate = (int)(((float)XBMC->GetFileLength(result->file) * 8) / ((float)(*totaltime)/ 1000));

  if (*samplerate == 0 || *channels == 0 || *bitspersample == 0 || *totaltime == 0)
  {
    XBMC->Log(ADDON::LOG_ERROR, "FLACCodec: Can't get stream info, SampleRate=%i, Channels=%i, BitsPerSample=%i, TotalTime=%"PRIu64", MaxFrameSize=%i", *samplerate, *channels, *bitspersample, *totaltime, result->framesize);
    delete result;
    return NULL;
  }

  //  allocate the buffer to hold the audio data,
  //  it is 5 times bigger then a single decoded frame
  result->buffer = new uint8_t[result->framesize*5];
  result->buffersize=0;

  return result;
}

int ReadPCM(void* context, uint8_t* pBuffer, int size, int *actualsize)
{
  *actualsize=0;

  FLACContext* ctx = (FLACContext*)context;

  bool eof=false;
  if (FLAC__stream_decoder_get_state(ctx->decoder)==FLAC__STREAM_DECODER_END_OF_STREAM)
    eof=true;

  if (!eof)
  {
    //  fill our buffer 4 decoded frame (the buffer could hold 5)
    while(ctx->buffersize < ctx->framesize*4 &&
          FLAC__stream_decoder_get_state(ctx->decoder)!=FLAC__STREAM_DECODER_END_OF_STREAM)
    {
      if (!FLAC__stream_decoder_process_single(ctx->decoder))
      {
        XBMC->Log(ADDON::LOG_ERROR, "FLACCodec: Error decoding single block");
        return 1;
      }
    }
  }

  if (size<ctx->buffersize)
  { //  do we need less audio data then in our buffer
    memcpy(pBuffer, ctx->buffer, size);
    memmove(ctx->buffer, ctx->buffer+size, ctx->buffersize-size);
    ctx->buffersize-=size;
    *actualsize=size;
  }
  else
  {
    memcpy(pBuffer, ctx->buffer, ctx->buffersize);
    *actualsize=ctx->buffersize;
    ctx->buffersize=0;
  }

  if (eof && ctx->buffersize==0)
    return -1;

  return 0;
}

int64_t Seek(void* context, int64_t time)
{
  FLACContext* ctx = (FLACContext*)context;
  //  Seek to the nearest sample
  // set the buffer size to 0 first, as this invokes a WriteCallback which
  // may be called when the buffer is almost full (resulting in a buffer
  // overrun unless we reset m_BufferSize first).
  ctx->buffersize=0;
  if(!FLAC__stream_decoder_seek_absolute(ctx->decoder, (int64_t)(time*ctx->samplerate)/1000))
    XBMC->Log(ADDON::LOG_ERROR, "FLACCodec::Seek - failed to seek");

  if(FLAC__stream_decoder_get_state(ctx->decoder)==FLAC__STREAM_DECODER_SEEK_ERROR)
  {
    XBMC->Log(ADDON::LOG_INFO, "FLACCodec::Seek - must reset decoder after seek");
    if(!FLAC__stream_decoder_flush(ctx->decoder))
      XBMC->Log(ADDON::LOG_ERROR, "FLACCodec::Seek - flush failed");
  }

  return time;
}

bool DeInit(void* context)
{
  delete (FLACContext*)context;
  return true;
}

bool ReadTag(const char* strFile, char* title, char* artist,
             int* length)
{
  return false;
}

int TrackCount(const char* strFile)
{
  return 1;
}
}
