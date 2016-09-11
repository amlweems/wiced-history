/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdbool.h>
#include <stdio.h>
#include <debug.h>
#include <assert.h>
#include <fcntl.h>

#include <pthread.h>
#include <mqueue.h>

#include <nuttx/kmalloc.h>
#include <nuttx/wqueue.h>
#include <nuttx/fs/fs.h>
#include <nuttx/fs/ioctl.h>
#include <nuttx/audio/audio.h>

#include <wiced.h>
#include "wiced_platform.h"
#include <wiced_audio.h>
#include <platform.h>

#define PERIOD_SIZE                 (1 * 1024)
#define NUM_PERIODS                 4
#define BUFFER_SIZE                 WICED_AUDIO_BUFFER_ARRAY_DIM_SIZEOF(NUM_PERIODS, PERIOD_SIZE)
#define SAMPLE_FREQUENCY_IN_HZ      (44100)

static int      bcm4390x_audio_getcaps(FAR struct audio_lowerhalf_s *dev, int type,
                  FAR struct audio_caps_s *caps);
static int      bcm4390x_audio_configure(FAR struct audio_lowerhalf_s *dev,
                  FAR const struct audio_caps_s *caps);
static int      bcm4390x_audio_shutdown(FAR struct audio_lowerhalf_s *dev);
static int      bcm4390x_audio_start(FAR struct audio_lowerhalf_s *dev);

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
static int      bcm4390x_audio_stop(FAR struct audio_lowerhalf_s *dev);
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
static int      bcm4390x_audio_pause(FAR struct audio_lowerhalf_s *dev);
static int      bcm4390x_audio_resume(FAR struct audio_lowerhalf_s *dev);
#endif
static int      bcm4390x_audio_allocbuffer(FAR struct audio_lowerhalf_s *dev,
                                       FAR struct audio_buf_desc_s *apb);
static int      bcm4390x_audio_freebuffer(FAR struct audio_lowerhalf_s *dev,
                                       FAR struct audio_buf_desc_s *apb);
static int      bcm4390x_audio_enqueuebuffer(FAR struct audio_lowerhalf_s *dev,
                  FAR struct ap_buffer_s *apb);
static int      bcm4390x_audio_cancelbuffer(FAR struct audio_lowerhalf_s *dev,
                  FAR struct ap_buffer_s *apb);
static int      bcm4390x_audio_ioctl(FAR struct audio_lowerhalf_s *dev, int cmd,
                  unsigned long arg);
static int      bcm4390x_audio_reserve(FAR struct audio_lowerhalf_s *dev);
static int      bcm4390x_audio_release(FAR struct audio_lowerhalf_s *dev);
static void    *bcm4390x_audio_workerthread(pthread_addr_t pvarg);

struct audio_lowerhalf_s* bcm4390x_audio_lowerhalf_init(void);
void bcm4390x_audio_lowerhalf_deinit(struct audio_lowerhalf_s *dev);

struct bcm4390x_audio_dev_s
{
  struct audio_lowerhalf_s dev;             /* bcm4390x audio lower half (this device) */
  /* Ensure that audio_lowerhaf_s is always the first field of this structure */

  /* Our specific driver data goes here */

  mqd_t                   mq;               /* Message queue for receiving messages */
  char                    mqname[16];       /* Our message queue name */
  pthread_t               threadid;         /* ID of our thread */
  uint32_t                bitrate;          /* Actual programmed bit rate */
  sem_t                   pendsem;          /* Protect pendq */
  uint16_t                samprate;         /* Configured samprate (samples/sec) */
#ifndef CONFIG_AUDIO_EXCLUDE_VOLUME
#ifndef CONFIG_AUDIO_EXCLUDE_BALANCE
  uint16_t                balance;          /* Current balance level (b16) */
#endif  /* CONFIG_AUDIO_EXCLUDE_BALANCE */
  uint8_t                 volume;           /* Current volume level {0..63} */
#endif  /* CONFIG_AUDIO_EXCLUDE_VOLUME */
  uint8_t                 nchannels;        /* Number of channels (1 or 2) */
  uint8_t                 bpsamp;           /* Bits per sample (8 or 16) */
  bool                    running;          /* True: Worker thread is running */
  bool                    paused;           /* True: Playing is paused */
  bool                    mute;             /* True: Output is muted */
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  bool                    terminating;      /* True: Stop requested */
#endif
  bool                    reserved;         /* True: Device is reserved */
  wiced_audio_session_ref tx_session;
  wiced_audio_session_ref rx_session;
  struct ap_buffer_s      apb[NUM_PERIODS];
};

static const struct audio_ops_s g_audioops =
{
  .getcaps          = bcm4390x_audio_getcaps,       /* getcaps        */
  .configure        = bcm4390x_audio_configure,     /* configure      */
  .shutdown         = bcm4390x_audio_shutdown,      /* shutdown       */
  .start            = bcm4390x_audio_start,         /* start          */
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  .stop             = bcm4390x_audio_stop,          /* stop           */
#endif
#ifndef CONFIG_AUDIO_EXCLUDE_PAUSE_RESUME
  .pause            = bcm4390x_audio_pause,         /* pause          */
  .resume           = bcm4390x_audio_resume,        /* resume         */
#endif
  .allocbuffer      = bcm4390x_audio_allocbuffer,   /* allocbuffer    */
  .freebuffer       = bcm4390x_audio_freebuffer,    /* freebuffer     */
  .enqueuebuffer    = bcm4390x_audio_enqueuebuffer, /* enqueue_buffer */
  .cancelbuffer     = bcm4390x_audio_cancelbuffer,  /* cancel_buffer  */
  .ioctl            = bcm4390x_audio_ioctl,         /* ioctl          */
  .read             = NULL,                         /* read           */
  .write            = NULL,                         /* write          */
  .reserve          = bcm4390x_audio_reserve,       /* reserve        */
  .release          = bcm4390x_audio_release        /* release        */
};

typedef struct
{
    /* Audio ring buffer. */
    wiced_audio_buffer_header_t *buffer;
    /* Start 0-index into the period. */
    uint16_t head;
    /* Bytes pending playback/capture transfer..
     * The inverse represents the bytes available to write (playback)
     * or read (captured).
    */
    uint16_t count;
    /* Total number of audio bytes in the buffer. */
    uint16_t length;
} audio_buffer_t;

struct wiced_audio_session_t
{
    int                             i2s_id;
    int                             i2s_direction;
    wiced_bool_t                    i2s_running;
    wiced_semaphore_t               available_periods;
    uint16_t                        num_periods_requested;
    uint8_t                         frame_size;
    audio_buffer_t                  audio_buffer;
    uint16_t                        period_size;
    wiced_audio_device_interface_t* audio_dev;
    wiced_mutex_t                   session_lock;
    wiced_bool_t                    underrun_occurred;
    wiced_bool_t                    is_initialised;
};

static wiced_audio_config_t wiced_audio_default_config =
{
    .sample_rate        = SAMPLE_FREQUENCY_IN_HZ,
    .channels           = 2,
    .bits_per_sample    = 16,
    .frame_size         = 4,
};

int bcm4390x_audio_initialize(void)
{
  struct audio_lowerhalf_s *bcm4390x_audio_lowerhalf;
  static bool initialized = false;
  char devname[20];
  int ret;
  wiced_result_t result;
  struct bcm4390x_audio_dev_s *priv;
  uint8_t *apb_buffer;

  if(!initialized)
  {
    result = platform_init_audio();
    if (result != WICED_SUCCESS)
    {
      auddbg("ERROR: platform_init_audio() failed");
      ret = -ENODEV;
      goto platform_init_audio_failed;
    }

    bcm4390x_audio_lowerhalf = bcm4390x_audio_lowerhalf_init();
    if (NULL == bcm4390x_audio_lowerhalf)
    {
      auddbg("ERROR: bcm4390x_audio_lowerhalf_init() failed");
      ret = -ENODEV;
      goto audio_lowerhalf_init_failed;
    }

    priv = (struct bcm4390x_audio_dev_s *)bcm4390x_audio_lowerhalf;

    result = wiced_audio_init(PLATFORM_DEFAULT_AUDIO_OUTPUT, &(priv->tx_session), PERIOD_SIZE);
    if (result != WICED_SUCCESS)
    {
      auddbg("ERROR: wiced_audio_init() failed");
      ret = -ENODEV;
      goto wiced_audio_init_failed;
    }

    /* Allocate Audio Buffer */
    apb_buffer = malloc(BUFFER_SIZE); /* Should be WICED_AUDIO_BUFFER_ALIGNMENT_BYTES aligned ? */
    if (apb_buffer == NULL)
    {
        auddbg("ERROR: malloc failed");
        ret = -ENOMEM;
        goto apb_buffer_alloc_failed;
    }

    result = wiced_audio_create_buffer(priv->tx_session, BUFFER_SIZE, apb_buffer, NULL);
    if (result != WICED_SUCCESS)
    {
        auddbg("ERROR: wiced_audio_create_buffer failed");
        ret = ERROR;
        goto wiced_audio_create_buffer_failed;
    }

    /* Configure Session */
    result = wiced_audio_configure(priv->tx_session, &wiced_audio_default_config);
    if (result != WICED_SUCCESS)
    {
      auddbg("ERROR: IOCTL AUDIO_TYPE_OUTPUT failed");
      ret = ERROR;
      goto wiced_audio_configure_failed;
    }

    /* Register with /dev/ interface */
    snprintf(devname, 20, "bcm4390x_audio");
    ret = audio_register(devname, bcm4390x_audio_lowerhalf);
    if (ret < 0)
      {
        auddbg("ERROR: Failed to register /dev/%s device: %d\n", devname, ret);
        ret = ERROR;
        goto audio_register_failed;
      }

    /* Now we are initialized */
    initialized = true;
  }

  return OK;

audio_register_failed:
wiced_audio_configure_failed:
wiced_audio_create_buffer_failed:
  wiced_audio_release_buffer(priv->tx_session, BUFFER_SIZE);
  free(apb_buffer);
apb_buffer_alloc_failed:
  wiced_audio_deinit(priv->tx_session);
wiced_audio_init_failed:
  bcm4390x_audio_lowerhalf_deinit((struct audio_lowerhalf_s *)priv);
audio_lowerhalf_init_failed:
  platform_deinit_audio();
platform_init_audio_failed:
  DEBUGASSERT(FALSE);
  return ret;
}

struct audio_lowerhalf_s* bcm4390x_audio_lowerhalf_init(void)
{
  struct bcm4390x_audio_dev_s *priv = NULL;

  priv = (struct bcm4390x_audio_dev_s*)kmm_zalloc(sizeof(struct bcm4390x_audio_dev_s));
  if (priv == NULL)
  {
      return NULL;
  }
  else
  {
    priv->dev.ops = &g_audioops;
    sem_init(&priv->pendsem, 0, 1);
    return &priv->dev;
  }
}

void bcm4390x_audio_lowerhalf_deinit(struct audio_lowerhalf_s *dev)
{
  struct bcm4390x_audio_dev_s *priv = (struct bcm4390x_audio_dev_s *)dev;
  if (priv != NULL)
  {
    kmm_free(priv);
    priv = NULL;
  }
}
/****************************************************************************
 * Name: bcm4390x_audio_getcaps
 *
 * Description:
 *   Get the audio device capabilities
 *
 ****************************************************************************/

static int bcm4390x_audio_getcaps(FAR struct audio_lowerhalf_s *dev, int type,
                          FAR struct audio_caps_s *caps)
{
  /* Validate the structure */

  DEBUGASSERT(caps && (caps->ac_len == sizeof(struct audio_caps_s)));

  /* Fill in the caller's structure based on requested info */

  caps->ac_format.hw  = 0;
  caps->ac_controls.w = 0;

  switch (caps->ac_type)
  {
      /* Caller is querying for the types of units we support */

      case AUDIO_TYPE_QUERY:

        /* Provide our overall capabilities.  The interfacing software
         * must then call us back for specific info for each capability.
         */

        caps->ac_channels = 2;       /* Stereo output */

        switch (caps->ac_subtype)
        {
            case AUDIO_TYPE_QUERY:
              /* We don't decode any formats!  Only something above us in
               * the audio stream can perform decoding on our behalf.
               */

              /* The types of audio units we implement */

              caps->ac_controls.b[0] = AUDIO_TYPE_OUTPUT | AUDIO_TYPE_FEATURE |
                                     AUDIO_TYPE_PROCESSING;

              break;

            case AUDIO_FMT_MIDI:
              /* We only support Format 0 */

              caps->ac_controls.b[0] = AUDIO_SUBFMT_END;
              break;

            default:
              caps->ac_controls.b[0] = AUDIO_SUBFMT_END;
              break;
        }

        break;

      /* Provide capabilities of our OUTPUT unit */

      case AUDIO_TYPE_OUTPUT:

        caps->ac_channels = 2;

        switch (caps->ac_subtype)
        {
            case AUDIO_TYPE_QUERY:

              /* Report the Sample rates we support */

              caps->ac_controls.b[0] = AUDIO_SAMP_RATE_8K | AUDIO_SAMP_RATE_11K |
                                       AUDIO_SAMP_RATE_16K | AUDIO_SAMP_RATE_22K |
                                       AUDIO_SAMP_RATE_32K | AUDIO_SAMP_RATE_44K |
                                       AUDIO_SAMP_RATE_48K;
              break;

            case AUDIO_FMT_MP3:
            case AUDIO_FMT_WMA:
            case AUDIO_FMT_PCM:
              break;

            default:
              break;
        }

        break;

      /* Provide capabilities of our FEATURE units */

      case AUDIO_TYPE_FEATURE:

        /* If the sub-type is UNDEF, then report the Feature Units we support */

        if (caps->ac_subtype == AUDIO_FU_UNDEF)
        {
            /* Fill in the ac_controls section with the Feature Units we have */

            caps->ac_controls.b[0] = AUDIO_FU_VOLUME | AUDIO_FU_BASS | AUDIO_FU_TREBLE;
            caps->ac_controls.b[1] = AUDIO_FU_BALANCE >> 8;
        }
        else
        {
            /* TODO:  Do we need to provide specific info for the Feature Units,
             * such as volume setting ranges, etc.?
             */
        }

        break;

      /* Provide capabilities of our PROCESSING unit */

      case AUDIO_TYPE_PROCESSING:

        switch (caps->ac_subtype)
        {
              default:
              break;
        }

        break;

      /* All others we don't support */

      default:

        /* Zero out the fields to indicate no support */

        caps->ac_subtype = 0;
        caps->ac_channels = 0;

        break;
  }
  /* Return the length of the audio_caps_s struct for validation of
   * proper Audio device type.
   */

  return caps->ac_len;
}

static int bcm4390x_audio_configure(FAR struct audio_lowerhalf_s *dev,
                            FAR const struct audio_caps_s *caps)
{
  FAR struct bcm4390x_audio_dev_s *priv = (FAR struct bcm4390x_audio_dev_s *)dev;
  int ret = OK;

  DEBUGASSERT(priv && caps);

  /* Process the configure operation */

  switch (caps->ac_type)
    {
    case AUDIO_TYPE_FEATURE:
      //audvdbg("  AUDIO_TYPE_FEATURE\n");

      /* Process based on Feature Unit */

      switch (caps->ac_format.hw)
        {
        case AUDIO_FU_VOLUME:
          {
            /* Set the volume */
            double minimum_volume, maximum_volume;
            wiced_audio_get_volume_range(priv->tx_session, &minimum_volume, &maximum_volume);
            wiced_audio_set_volume(priv->tx_session, maximum_volume);
          }
          break;

        default:
          auddbg("    Unrecognized feature unit\n");
          ret = -ENOTTY;
          break;
        }
        break;

    case AUDIO_TYPE_OUTPUT:
      {
        wiced_audio_config_t config;
        wiced_result_t result;

        config.sample_rate     = caps->ac_controls.hw[0];
        config.channels        = caps->ac_channels;
        config.bits_per_sample = caps->ac_controls.b[2];
        config.frame_size      = (config.channels * config.bits_per_sample)/8;

        result = wiced_audio_configure(priv->tx_session, &config);
        if (result != WICED_SUCCESS)
        {
          auddbg("ERROR IOCTL AUDIO_TYPE_OUTPUT failed");
          DEBUGASSERT(FALSE);
          return ERROR;
        }

        /* Save the current stream configuration */
        priv->samprate  = caps->ac_controls.hw[0];
        priv->nchannels = caps->ac_channels;
        priv->bpsamp    = caps->ac_controls.b[2];

        ret = OK;
      }
      break;

    case AUDIO_TYPE_PROCESSING:
      break;
    }

  return ret;
}

/****************************************************************************
 * Name: bcm4390x_audio_start
 *
 * Description:
 *   Start the configured operation (audio streaming, volume enabled, etc.).
 *
 ****************************************************************************/

static int bcm4390x_audio_start(FAR struct audio_lowerhalf_s *dev)
{
  FAR struct bcm4390x_audio_dev_s *priv = (FAR struct bcm4390x_audio_dev_s *)dev;
  struct sched_param sparam;
  struct mq_attr attr;
  pthread_attr_t tattr;
  FAR void *value;
  int ret;


  /* Create a message queue for the worker thread */
  snprintf(priv->mqname, sizeof(priv->mqname), "/tmp/%X", priv);

  attr.mq_maxmsg  = 16;
  attr.mq_msgsize = sizeof(struct audio_msg_s);
  attr.mq_curmsgs = 0;
  attr.mq_flags   = 0;

  priv->mq = mq_open(priv->mqname, O_RDWR | O_CREAT, 0644, &attr);
  if (priv->mq == NULL)
    {
      /* Error creating message queue! */

      auddbg("ERROR: Couldn't allocate message queue\n");
      return -ENOMEM;
    }

  /* Join any old worker thread we had created to prevent a memory leak */

  if (priv->threadid != 0)
    {
      audvdbg("Joining old thread\n");
      pthread_join(priv->threadid, &value);
    }

  /* Start our thread for sending data to the device */

  pthread_attr_init(&tattr);
  sparam.sched_priority = sched_get_priority_max(SCHED_FIFO);
  (void)pthread_attr_setschedparam(&tattr, &sparam);
  (void)pthread_attr_setstacksize(&tattr, 1536);

  ret = pthread_create(&priv->threadid, &tattr, bcm4390x_audio_workerthread,
                       (pthread_addr_t)priv);
  if (ret != OK)
    {
      auddbg("ERROR: pthread_create failed: %d\n", ret);
    }
  else
    {
      pthread_setname_np(priv->threadid, "bcm4390x_audio");
    }

  return ret;
}

/****************************************************************************
 * Name: bcm4390x_audio_stop
 *
 * Description: Stop the configured operation (audio streaming, volume
 *              disabled, etc.).
 *
 ****************************************************************************/

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
static int bcm4390x_audio_stop(FAR struct audio_lowerhalf_s *dev)
{
  struct bcm4390x_audio_dev_s *priv = (struct bcm4390x_audio_dev_s *)dev;
  struct audio_msg_s term_msg;
  void *value;

  /* Send a message to stop all audio streaming */

  term_msg.msgId = AUDIO_MSG_STOP;
  term_msg.u.data = 0;
  mq_send(priv->mq, (FAR const char *)&term_msg, sizeof(term_msg),
          1);

  /* Join the worker thread */

  pthread_join(priv->threadid, &value);
  priv->threadid = 0;

  /* Enter into a reduced power usage mode */
  /* REVISIT: */

  return OK;
}
#endif

static int bcm4390x_audio_shutdown(FAR struct audio_lowerhalf_s *dev)
{
  /* To be implemented */
  return OK;
}

static int bcm4390x_audio_allocbuffer(FAR struct audio_lowerhalf_s *dev,
                                      FAR struct audio_buf_desc_s *apb)
{
  static int index = 0;
  int count;
  struct bcm4390x_audio_dev_s *priv = (struct bcm4390x_audio_dev_s *)dev;
  wiced_audio_buffer_header_t *buffer_ptr = NULL;
  struct ap_buffer_s *apb_ptr = &(priv->apb[index]);

  buffer_ptr = priv->tx_session->audio_buffer.buffer;
  count = index;

  while( count != 0)
  {
    buffer_ptr = buffer_ptr->next;
    count--;
  }

  apb_ptr->samp = buffer_ptr->data_start;
  apb_ptr->nmaxbytes = buffer_ptr->data_end - buffer_ptr->data_start;

  *apb->u.ppBuffer = apb_ptr;
  index ++;

  return sizeof(struct audio_buf_desc_s);
}

static int bcm4390x_audio_freebuffer(FAR struct audio_lowerhalf_s *dev,
                                     FAR struct audio_buf_desc_s *apb)
{
  /* TODO take semaphore, reference count to count how many users */
   return OK;
}


/****************************************************************************
 * Name: bcm4390x_audio__enqueuebuffer
 *
 * Description: Enqueue an Audio Pipeline Buffer for playback/ processing.
 *
 ****************************************************************************/

static int bcm4390x_audio_enqueuebuffer(FAR struct audio_lowerhalf_s *dev,
                                FAR struct ap_buffer_s *apb)
{

  int ret = OK;
  wiced_result_t result;

  FAR struct bcm4390x_audio_dev_s *priv = (FAR struct bcm4390x_audio_dev_s *)dev;
  struct audio_msg_s  term_msg;

  result = wiced_audio_release_buffer(priv->tx_session, PERIOD_SIZE); /*TODO Change PERIOD_SIZE to size of buffer */
  if (result != WICED_SUCCESS)
  {
    auddbg("wiced_audio_release_buffer_failed\n");
    DEBUGASSERT(FALSE);
    return ERROR;
  }

  /* Send a message to the worker thread indicating that a new buffer has been
   * enqueued.  If mq is NULL, then the playing has not yet started.  In that
   * case we are just "priming the pump" and we don't need to send any message.
   */

  ret = OK;
  if (priv->mq != NULL)
    {
      term_msg.msgId  = AUDIO_MSG_ENQUEUE;
      term_msg.u.data = 0;

      ret = mq_send(priv->mq, (FAR const char *)&term_msg, sizeof(term_msg),
                    1);
      if (ret < 0)
        {
          int errcode = errno;
          DEBUGASSERT(errcode > 0);

          auddbg("ERROR: mq_send failed: %d\n", errcode);
          UNUSED(errcode);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: bcm4390x_audio_cancelbuffer
 *
 * Description: Called when an enqueued buffer is being cancelled.
 *
 ****************************************************************************/

static int bcm4390x_audio_cancelbuffer(FAR struct audio_lowerhalf_s *dev,
                               FAR struct ap_buffer_s *apb)
{
  return OK;
}

static int bcm4390x_audio_ioctl(FAR struct audio_lowerhalf_s *dev, int cmd,
                        unsigned long arg)
{
  switch (cmd)
  {
#ifdef CONFIG_AUDIO_DRIVER_SPECIFIC_BUFFERS
    case AUDIOIOC_GETBUFFERINFO:
    {
      struct ap_buffer_info_s *bufinfo;
      bufinfo              = (struct ap_buffer_info_s *) arg;
      bufinfo->buffer_size = BUFFER_SIZE;
      bufinfo->nbuffers    = NUM_PERIODS;
    }
#endif
    default:
      break;
  }

  return OK;
}

static int bcm4390x_audio_reserve(FAR struct audio_lowerhalf_s *dev)
{
  struct bcm4390x_audio_dev_s *priv = (FAR struct bcm4390x_audio_dev_s *) dev;
  int   ret = OK;

  priv->running     = false;
  priv->paused      = false;
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  priv->terminating = false;
#endif
  priv->reserved    = true;

  return ret;

}

static int bcm4390x_audio_release(FAR struct audio_lowerhalf_s *dev)
{
  struct bcm4390x_audio_dev_s *priv = (FAR struct bcm4390x_audio_dev_s *) dev;
  int   ret = OK;

  priv->reserved    = FALSE;
  return ret;

}

static int bcm4390x_audio_wait_for_send_complete(struct bcm4390x_audio_dev_s *priv)
{
  wiced_result_t result;
  uint16_t avail = PERIOD_SIZE;
  uint8_t *buf;
  struct ap_buffer_s *apb = &(priv->apb[0]);

  result = wiced_audio_wait_buffer(priv->tx_session, PERIOD_SIZE, 1000);
  if (result != WICED_SUCCESS)
  {
    printf("Timed out waiting for audio buffer\n");
    DEBUGASSERT(FALSE);
    return ERROR;
  }

  result = wiced_audio_get_buffer(priv->tx_session, &buf, &avail);
  DEBUGASSERT(avail == PERIOD_SIZE);
  if (result != WICED_SUCCESS)
  {
    printf("wiced_audio_get_buffer() returned error\n");
    DEBUGASSERT(FALSE);
    return ERROR;
  }

  apb->nmaxbytes = avail;
  apb->samp = buf;

  priv->dev.upper(priv->dev.priv, AUDIO_CALLBACK_DEQUEUE, apb, OK);

  return OK;
}

static void *bcm4390x_audio_workerthread(pthread_addr_t pvarg)
{
  FAR struct bcm4390x_audio_dev_s *priv = (struct bcm4390x_audio_dev_s *) pvarg;
  struct audio_msg_s msg;
  int msglen;
  int prio;

  //audvdbg("Entry\n");

#ifndef CONFIG_AUDIO_EXCLUDE_STOP
  priv->terminating = false;
#endif

  if (WICED_SUCCESS != wiced_audio_start(priv->tx_session))
  {
      auddbg("ERROR: wiced_audio_Start failed\n");
      return NULL;

  }

  priv->running = true;

  /* Loop as long as we are supposed to be running and as long as we have
   * buffers in-flight.
   */
  while (priv->running)
    {

      bcm4390x_audio_wait_for_send_complete(priv);

      /* Check if we have been asked to terminate.  We have to check if we
       * still have buffers in-flight.  If we do, then we can't stop until
       * birds come back to roost.
       */
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
      if (priv->terminating)
        {
          /* We are IDLE.  Break out of the loop and exit. */

          break;
        }
#endif

      /* Wait for messages from our message queue */
      msglen = mq_receive(priv->mq, (FAR char *)&msg, sizeof(msg), &prio);

      /* Handle the case when we return with no message */
      if (msglen < sizeof(struct audio_msg_s))
        {
          auddbg("ERROR: Message too small: %d\n", msglen);
          continue;
        }

      /* Process the message */
      switch (msg.msgId)
        {
          case AUDIO_MSG_DATA_REQUEST:
            //audvdbg("AUDIO_MSG_DATA_REQUEST\n");
            break;

          /* Stop the playback */
#ifndef CONFIG_AUDIO_EXCLUDE_STOP
          case AUDIO_MSG_STOP:
            /* Indicate that we are terminating */

            audvdbg("AUDIO_MSG_STOP: Terminating\n");
            priv->terminating = true;
            break;
#endif

          case AUDIO_MSG_ENQUEUE:
            break;

          case AUDIO_MSG_COMPLETE:
            break;

          default:
            audvdbg("ERROR: Ignoring message ID %d\n", msg.msgId);
            break;
        }

    }

  mq_close(priv->mq);
  mq_unlink(priv->mqname);
  priv->mq = NULL;

  /* Send an AUDIO_MSG_COMPLETE message to the client */
  priv->dev.upper(priv->dev.priv, AUDIO_CALLBACK_COMPLETE, NULL, OK);

  audvdbg("Exit\n");
  return NULL;
}
