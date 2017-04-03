<?php
/**
 * @file
 * File containing class TranscoderAbstractionFactoryFfmpeg
 */

/**
 * Class that handles fake transcoding.
 */
class TranscoderAbstractionFactoryStub extends TranscoderAbstractionFactory implements TranscoderFactoryInterface {

  protected $debug;

  function __construct() {
    $this->debug = variable_get('transcoderstub_debug_enabled', FALSE);
  }

  /**
   * Extract frames from the current video.
   *
   * The thumbnails are be saved to the given scheme. The files are not
   * be saved to the database, this will be done by the caller.
   * The uid, filesize and timestamp properties are not set by this method.
   *
   * @param $destination_scheme
   *   A valid stream wrapper scheme
   * @param $format
   *   png or jpg
   *
   * @return bool array of file objects, FALSE on error
   */
  public function extractFrames($destination_scheme, $format) {
    if ($this->debug) {
      watchdog('transcoderstub', '@method called. Settings: <pre>@data</pre>', array('@method' => __METHOD__, '@data' => print_r($this->settings, TRUE)), WATCHDOG_DEBUG);
    }
    $thumbs = array();
    $no_of_thumbnails = variable_get('video_thumbnail_count', 5);
    if (!empty($this->settings['input']['fid'])) {
      $fid = intval($this->settings['input']['fid']);
      if (!empty($this->settings['input']['dimensions'])) {
        $dimensions = $this->settings['input']['dimensions'];
      }
      else {
        $dimensions = '640x480';
        watchdog('transcoderstub', 'Input dimensions not set. Using @dimensions.', array('@dimensions' => $dimensions), WATCHDOG_WARNING);
      }
      $x_by_y = explode('x', drupal_strtolower($dimensions));
      if (count($x_by_y) == 2) {

        // Get the file system directory.
        $destination_directory = $destination_scheme . '://' . variable_get('video_thumbnail_path', 'videos/thumbnails') . '/' . $fid . '/';
        $writable = file_prepare_directory($destination_directory, FILE_CREATE_DIRECTORY);
        if ($writable) {
          $streamwrapper = file_stream_wrapper_get_instance_by_scheme($destination_scheme);
          if ($streamwrapper) {
            for ($i = 1; $i <= $no_of_thumbnails; $i++) {
              $filename = file_munge_filename('thumbnail-' . $fid . '_' . sprintf('%04d', $i) . '.' . $format, '', FALSE);
              $destination = $destination_directory . $filename;
              if (!file_exists($destination)) {
                if ($this->debug) {
                  $args = array(
                    '@dimensions' => $dimensions,
                    '@format' => $format,
                    '@filename' => $filename,
                    '@fid' => $fid,
                  );
                  watchdog('transcoderstub', 'Creating fake @dimensions @format thumbnail image @filename for video file @fid', $args, WATCHDOG_DEBUG);
                }
                $width = $x_by_y[0];
                $height = $x_by_y[1];

                // Make an image split into 4 sections with random colors.
                $im = imagecreate($width, $height);
                for ($n = 0; $n < 4; $n++) {
                  $color = imagecolorallocate($im, rand(0, 255), rand(0, 255), rand(0, 255));
                  $x = $width / 2 * ($n % 2);
                  $y = $height / 2 * (int) ($n >= 2);
                  imagefilledrectangle($im, $x, $y, $x + $width / 2, $y + $height / 2, $color);
                }

                // Make a perfect circle in the image middle.
                $color = imagecolorallocate($im, rand(0, 255), rand(0, 255), rand(0, 255));
                $smaller_dimension = min($width, $height);
                $smaller_dimension = ($smaller_dimension % 2) ? $smaller_dimension : $smaller_dimension;
                imageellipse($im, $width / 2, $height / 2, $smaller_dimension, $smaller_dimension, $color);

                $save_function = 'image' . ($format == 'jpg' ? 'jpeg' : $format);
                $save_function($im, drupal_realpath($destination));

              }
              // Begin building the file object.
              $thumb = new stdClass();
              $thumb->status = 0;
              $thumb->filename = $filename;
              $thumb->uri = $destination;
              $thumb->filemime = $streamwrapper->getMimeType($destination);
              $thumbs[] = $thumb;
            }
          }
          else {
            watchdog('transcoderstub', 'No stream wrapper could be found for @scheme.', array('@scheme' => $destination_scheme), WATCHDOG_ERROR);
          }
        }
        else {
          watchdog('transcoderstub', 'Destination directory @directory is not writable.', array('@directory' => $destination_directory), WATCHDOG_ERROR);
        }
      }
      else {
        watchdog('transcoderstub', 'Invalid dimensions @dimensions.', array('@dimensions' => $dimensions), WATCHDOG_ERROR);
      }
    }
    else {
      watchdog('transcoderstub', 'Input file id not set.', array(), WATCHDOG_ERROR);
    }
    return $thumbs;
  }

  /**
   * @return bool|stdClass|True
   */
  public function execute() {
    if ($this->debug) {
      watchdog('transcoderstub', '@method called. Settings: <pre>@data</pre>', array('@method' => __METHOD__, '@data' => print_r($this->settings, TRUE)), WATCHDOG_DEBUG);
    }
    // Return a copy of the original file.
    if (!empty($this->settings['input']['fid'])) {
      $source = file_load($this->settings['input']['fid']);
      if ($source) {
        if (!empty($this->settings['base_url'])) {
          if (!empty($this->settings['filename'])) {
            $destination = $this->settings['base_url'] . '/' . $this->settings['filename'];
            $output = file_copy($source, $destination, FILE_EXISTS_REPLACE);
            if ($output) {
              $output->timestamp = REQUEST_TIME;
              $output->jobid = NULL;
              $output->duration = empty($this->settings['input']['filesize']) ? 0 : $this->settings['input']['filesize'] / 500000;
              sleep(variable_get('transcoderstub_delay', 0));
              return $output;
            }
            else {
              watchdog('transcoderstub', 'Unable to copy file to @destination.', array('@destination' => $destination), WATCHDOG_ERROR);
            }
          }
          else {
            watchdog('transcoderstub', 'Destination filename not set.', array(), WATCHDOG_ERROR);
          }
        }
        else {
          watchdog('transcoderstub', 'Destination location (base_url) not set.', array(), WATCHDOG_ERROR);
        }
      }
      else {
        watchdog('transcoderstub', 'Could not load input file with id @id.', array('@id' => $this->settings['input']['fid']), WATCHDOG_ERROR);
      }
    }
    else {
      watchdog('transcoderstub', 'No input file id defined.', array(), WATCHDOG_ERROR);
    }
  }

  /**
   * @return string
   */
  public function getName() {
    return 'Stub transcoder for testing';
  }

  /**
   * @return string
   */
  public function getValue() {
    return 'TranscoderAbstractionFactoryStub';
  }

  /**
   * Get enabled and supporting codecs by the transcoder.
   */
  public function getCodecs() {
    return array(
      'decode' => array(
        'video' => array(
          '012v' => 'Uncompressed 4:2:2 10-bit',
          '4xm' => '4X Movie',
          '8bps' => 'QuickTime 8BPS video',
          'aasc' => 'Autodesk RLE',
          'amv' => 'AMV Video',
          'anm' => 'Deluxe Paint Animation',
          'ansi' => 'ASCII/ANSI art',
          'asv1' => 'ASUS V1',
          'asv2' => 'ASUS V2',
          'aura' => 'Auravision AURA',
          'aura2' => 'Auravision Aura 2',
          'avrn' => 'Avid AVI Codec',
          'avrp' => 'Avid 1:1 10-bit RGB Packer',
          'avs' => 'AVS (Audio Video Standard) video',
          'avui' => 'Avid Meridien Uncompressed',
          'ayuv' => 'Uncompressed packed MS 4:4:4:4',
          'bethsoftvid' => 'Bethesda VID video',
          'bfi' => 'Brute Force & Ignorance',
          'binkvideo' => 'Bink video',
          'bintext' => 'Binary text',
          'bmp' => 'BMP (Windows and OS/2 bitmap)',
          'bmv_video' => 'Discworld II BMV video',
          'brender_pix' => 'BRender PIX image',
          'c93' => 'Interplay C93',
          'cavs' => 'Chinese AVS (Audio Video Standard) (AVS1-P2, JiZhun profile)',
          'cdgraphics' => 'CD Graphics video',
          'cdxl' => 'Commodore CDXL video',
          'cinepak' => 'Cinepak',
          'cljr' => 'Cirrus Logic AccuPak',
          'cllc' => 'Canopus Lossless Codec',
          'cmv' => 'Electronic Arts CMV video (decoders: eacmv )',
          'cpia' => 'CPiA video format',
          'cscd' => 'CamStudio (decoders: camstudio )',
          'cyuv' => 'Creative YUV (CYUV)',
          'dfa' => 'Chronomaster DFA',
          'dirac' => 'Dirac',
          'dnxhd' => 'VC3/DNxHD',
          'dpx' => 'DPX image',
          'dsicinvideo' => 'Delphine Software International CIN video',
          'dvvideo' => 'DV (Digital Video)',
          'dxa' => 'Feeble Files/ScummVM DXA',
          'dxtory' => 'Dxtory',
          'escape124' => 'Escape 124',
          'escape130' => 'Escape 130',
          'exr' => 'OpenEXR image',
          'ffv1' => 'FFmpeg video codec #1',
          'ffvhuff' => 'Huffyuv FFmpeg variant',
          'flashsv' => 'Flash Screen Video v1',
          'flashsv2' => 'Flash Screen Video v2',
          'flic' => 'Autodesk Animator Flic video',
          'flv1' => 'FLV / Sorenson Spark / Sorenson H.263 (Flash Video) (decoders: flv ) (encoders: flv )',
          'fraps' => 'Fraps',
          'frwu' => 'Forward Uncompressed',
          'gif' => 'GIF (Graphics Interchange Format)',
          'h261' => 'H.261',
          'h263' => 'H.263 / H.263-1996, H.263+ / H.263-1998 / H.263 version 2',
          'h263i' => 'Intel H.263',
          'h263p' => 'H.263+ / H.263-1998 / H.263 version 2',
          'h264' => 'H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (encoders: libx264 libx264rgb )',
          'huffyuv' => 'HuffYUV',
          'idcin' => 'id Quake II CIN video (decoders: idcinvideo )',
          'idf' => 'iCEDraw text',
          'iff_byterun1' => 'IFF ByteRun1 (decoders: iff )',
          'iff_ilbm' => 'IFF ILBM (decoders: iff )',
          'indeo2' => 'Intel Indeo 2',
          'indeo3' => 'Intel Indeo 3',
          'indeo4' => 'Intel Indeo Video Interactive 4',
          'indeo5' => 'Intel Indeo Video Interactive 5',
          'interplayvideo' => 'Interplay MVE video',
          'jpeg2000' => 'JPEG 2000 (decoders: j2k ) (encoders: j2k )',
          'jpegls' => 'JPEG-LS',
          'jv' => 'Bitmap Brothers JV video',
          'kgv1' => 'Kega Game Video',
          'kmvc' => "Karl Morton's video codec",
          'lagarith' => 'Lagarith lossless',
          'loco' => 'LOCO',
          'mad' => 'Electronic Arts Madcow Video (decoders: eamad )',
          'mdec' => 'Sony PlayStation MDEC (Motion DECoder)',
          'mimic' => 'Mimic',
          'mjpeg' => 'Motion JPEG',
          'mjpegb' => 'Apple MJPEG-B',
          'mmvideo' => 'American Laser Games MM Video',
          'motionpixels' => 'Motion Pixels video',
          'mpeg1video' => 'MPEG-1 video',
          'mpeg2video' => 'MPEG-2 video (decoders: mpeg2video mpegvideo )',
          'mpeg4' => 'MPEG-4 part 2 (encoders: mpeg4 libxvid )',
          'msa1' => 'MS ATC Screen',
          'msmpeg4v1' => 'MPEG-4 part 2 Microsoft variant version 1',
          'msmpeg4v2' => 'MPEG-4 part 2 Microsoft variant version 2',
          'msmpeg4v3' => 'MPEG-4 part 2 Microsoft variant version 3 (decoders: msmpeg4 ) (encoders: msmpeg4 )',
          'msrle' => 'Microsoft RLE',
          'mss1' => 'MS Screen 1',
          'mss2' => 'MS Windows Media Video V9 Screen',
          'msvideo1' => 'Microsoft Video 1',
          'mszh' => 'LCL (LossLess Codec Library) MSZH',
          'mts2' => 'MS Expression Encoder Screen',
          'mvc1' => 'Silicon Graphics Motion Video Compressor 1',
          'mvc2' => 'Silicon Graphics Motion Video Compressor 2',
          'mxpeg' => 'Mobotix MxPEG video',
          'nuv' => 'NuppelVideo/RTJPEG',
          'paf_video' => 'Amazing Studio Packed Animation File Video',
          'pam' => 'PAM (Portable AnyMap) image',
          'pbm' => 'PBM (Portable BitMap) image',
          'pcx' => 'PC Paintbrush PCX image',
          'pgm' => 'PGM (Portable GrayMap) image',
          'pgmyuv' => 'PGMYUV (Portable GrayMap YUV) image',
          'pictor' => 'Pictor/PC Paint',
          'png' => 'PNG (Portable Network Graphics) image',
          'ppm' => 'PPM (Portable PixelMap) image',
          'prores' => 'Apple ProRes (iCodec Pro) (decoders: prores prores_lgpl ) (encoders: prores prores_anatoliy prores_kostya )',
          'ptx' => 'V.Flash PTX image',
          'qdraw' => 'Apple QuickDraw',
          'qpeg' => 'Q-team QPEG',
          'qtrle' => 'QuickTime Animation (RLE) video',
          'r10k' => 'AJA Kona 10-bit RGB Codec',
          'r210' => 'Uncompressed RGB 10-bit',
          'rawvideo' => 'raw video',
          'rl2' => 'RL2 video',
          'roq' => 'id RoQ video (decoders: roqvideo ) (encoders: roqvideo )',
          'rpza' => 'QuickTime video (RPZA)',
          'rv10' => 'RealVideo 1.0',
          'rv20' => 'RealVideo 1.0',
          'rv30' => 'RealVideo 3.0',
          'rv40' => 'RealVideo 4.0',
          'sanm' => 'LucasArts SMUSH video',
          'sgi' => 'SGI image',
          'sgirle' => 'SGI RLE 8-bit',
          'smackvideo' => 'Smacker video (decoders: smackvid )',
          'smc' => 'QuickTime Graphics (SMC)',
          'snow' => 'Snow',
          'sp5x' => 'Sunplus JPEG (SP5X)',
          'sunrast' => 'Sun Rasterfile image',
          'svq1' => 'Sorenson Vector Quantizer 1 / Sorenson Video 1 / SVQ1',
          'svq3' => 'Sorenson Vector Quantizer 3 / Sorenson Video 3 / SVQ3',
          'targa' => 'Truevision Targa image',
          'targa_y216' => 'Pinnacle TARGA CineWave YUV16',
          'tgq' => 'Electronic Arts TGQ video (decoders: eatgq )',
          'tgv' => 'Electronic Arts TGV video (decoders: eatgv )',
          'theora' => 'Theora (encoders: libtheora )',
          'thp' => 'Nintendo Gamecube THP video',
          'tiertexseqvideo' => 'Tiertex Limited SEQ video',
          'tiff' => 'TIFF image',
          'tmv' => '8088flex TMV',
          'tqi' => 'Electronic Arts TQI video (decoders: eatqi )',
          'truemotion1' => 'Duck TrueMotion 1.0',
          'truemotion2' => 'Duck TrueMotion 2.0',
          'tscc' => 'TechSmith Screen Capture Codec (decoders: camtasia )',
          'tscc2' => 'TechSmith Screen Codec 2',
          'txd' => 'Renderware TXD (TeXture Dictionary) image',
          'ulti' => 'IBM UltiMotion (decoders: ultimotion )',
          'utvideo' => 'Ut Video',
          'v210' => 'Uncompressed 4:2:2 10-bit',
          'v308' => 'Uncompressed packed 4:4:4',
          'v408' => 'Uncompressed packed QT 4:4:4:4',
          'v410' => 'Uncompressed 4:4:4 10-bit',
          'vb' => 'Beam Software VB',
          'vble' => 'VBLE Lossless Codec',
          'vc1' => 'SMPTE VC-1',
          'vc1image' => 'Windows Media Video 9 Image v2',
          'vcr1' => 'ATI VCR1',
          'vixl' => 'Miro VideoXL (decoders: xl )',
          'vmdvideo' => 'Sierra VMD video',
          'vmnc' => 'VMware Screen Codec / VMware Video',
          'vp3' => 'On2 VP3',
          'vp5' => 'On2 VP5',
          'vp6' => 'On2 VP6',
          'vp6a' => 'On2 VP6 (Flash version, with alpha channel)',
          'vp6f' => 'On2 VP6 (Flash version)',
          'vp8' => 'On2 VP8 (decoders: vp8 libvpx ) (encoders: libvpx )',
          'wmv1' => 'Windows Media Video 7',
          'wmv2' => 'Windows Media Video 8',
          'wmv3' => 'Windows Media Video 9',
          'wmv3image' => 'Windows Media Video 9 Image',
          'wnv1' => 'Winnov WNV1',
          'ws_vqa' => 'Westwood Studios VQA (Vector Quantized Animation) video (decoders: vqavideo )',
          'xan_wc3' => 'Wing Commander III / Xan',
          'xan_wc4' => 'Wing Commander IV / Xxan',
          'xbin' => 'eXtended BINary text',
          'xbm' => 'XBM (X BitMap) image',
          'xface' => 'X-face image',
          'xwd' => 'XWD (X Window Dump) image',
          'y41p' => 'Uncompressed YUV 4:1:1 12-bit',
          'yop' => 'Psygnosis YOP Video',
          'yuv4' => 'Uncompressed packed 4:2:0',
          'zerocodec' => 'ZeroCodec Lossless Video',
          'zlib' => 'LCL (LossLess Codec Library) ZLIB',
          'zmbv' => 'Zip Motion Blocks Video',
        ),
        'audio' => array(
          '8svx_exp' => '8SVX exponential',
          '8svx_fib' => '8SVX fibonacci',
          'aac' => 'AAC (Advanced Audio Coding) (encoders: aac libfaac )',
          'aac_latm' => 'AAC LATM (Advanced Audio Coding LATM syntax)',
          'ac3' => 'ATSC A/52A (AC-3) (encoders: ac3 ac3_fixed )',
          'adpcm_4xm' => 'ADPCM 4X Movie',
          'adpcm_adx' => 'SEGA CRI ADX ADPCM',
          'adpcm_afc' => 'ADPCM Nintendo Gamecube AFC',
          'adpcm_ct' => 'ADPCM Creative Technology',
          'adpcm_ea' => 'ADPCM Electronic Arts',
          'adpcm_ea_maxis_xa' => 'ADPCM Electronic Arts Maxis CDROM XA',
          'adpcm_ea_r1' => 'ADPCM Electronic Arts R1',
          'adpcm_ea_r2' => 'ADPCM Electronic Arts R2',
          'adpcm_ea_r3' => 'ADPCM Electronic Arts R3',
          'adpcm_ea_xas' => 'ADPCM Electronic Arts XAS',
          'adpcm_g722' => 'G.722 ADPCM (decoders: g722 ) (encoders: g722 )',
          'adpcm_g726' => 'G.726 ADPCM (decoders: g726 ) (encoders: g726 )',
          'adpcm_ima_amv' => 'ADPCM IMA AMV',
          'adpcm_ima_apc' => 'ADPCM IMA CRYO APC',
          'adpcm_ima_dk3' => 'ADPCM IMA Duck DK3',
          'adpcm_ima_dk4' => 'ADPCM IMA Duck DK4',
          'adpcm_ima_ea_eacs' => 'ADPCM IMA Electronic Arts EACS',
          'adpcm_ima_ea_sead' => 'ADPCM IMA Electronic Arts SEAD',
          'adpcm_ima_iss' => 'ADPCM IMA Funcom ISS',
          'adpcm_ima_oki' => 'ADPCM IMA Dialogic OKI',
          'adpcm_ima_qt' => 'ADPCM IMA QuickTime',
          'adpcm_ima_smjpeg' => 'ADPCM IMA Loki SDL MJPEG',
          'adpcm_ima_wav' => 'ADPCM IMA WAV',
          'adpcm_ima_ws' => 'ADPCM IMA Westwood',
          'adpcm_ms' => 'ADPCM Microsoft',
          'adpcm_sbpro_2' => 'ADPCM Sound Blaster Pro 2-bit',
          'adpcm_sbpro_3' => 'ADPCM Sound Blaster Pro 2.6-bit',
          'adpcm_sbpro_4' => 'ADPCM Sound Blaster Pro 4-bit',
          'adpcm_swf' => 'ADPCM Shockwave Flash',
          'adpcm_thp' => 'ADPCM Nintendo Gamecube THP',
          'adpcm_xa' => 'ADPCM CDROM XA',
          'adpcm_yamaha' => 'ADPCM Yamaha',
          'alac' => 'ALAC (Apple Lossless Audio Codec)',
          'amr_nb' => 'AMR-NB (Adaptive Multi-Rate NarrowBand) (decoders: amrnb libopencore_amrnb ) (encoders: libopencore_amrnb )',
          'amr_wb' => 'AMR-WB (Adaptive Multi-Rate WideBand) (decoders: amrwb libopencore_amrwb )',
          'ape' => "Monkey's Audio",
          'atrac1' => 'Atrac 1 (Adaptive TRansform Acoustic Coding)',
          'atrac3' => 'Atrac 3 (Adaptive TRansform Acoustic Coding 3)',
          'binkaudio_dct' => 'Bink Audio (DCT)',
          'binkaudio_rdft' => 'Bink Audio (RDFT)',
          'bmv_audio' => 'Discworld II BMV audio',
          'comfortnoise' => 'RFC 3389 Comfort Noise',
          'cook' => 'Cook / Cooker / Gecko (RealAudio G2)',
          'dsicinaudio' => 'Delphine Software International CIN audio',
          'dts' => 'DCA (DTS Coherent Acoustics) (decoders: dca ) (encoders: dca )',
          'eac3' => 'ATSC A/52B (AC-3, E-AC-3)',
          'flac' => 'FLAC (Free Lossless Audio Codec)',
          'g723_1' => 'G.723.1',
          'g729' => 'G.729',
          'gsm' => 'GSM',
          'gsm_ms' => 'GSM Microsoft variant',
          'iac' => 'IAC (Indeo Audio Coder)',
          'imc' => 'IMC (Intel Music Coder)',
          'interplay_dpcm' => 'DPCM Interplay',
          'mace3' => 'MACE (Macintosh Audio Compression/Expansion) 3:1',
          'mace6' => 'MACE (Macintosh Audio Compression/Expansion) 6:1',
          'mlp' => 'MLP (Meridian Lossless Packing)',
          'mp1' => 'MP1 (MPEG audio layer 1) (decoders: mp1 mp1float )',
          'mp2' => 'MP2 (MPEG audio layer 2) (decoders: mp2 mp2float )',
          'mp3' => 'MP3 (MPEG audio layer 3) (decoders: mp3 mp3float ) (encoders: libmp3lame )',
          'mp3adu' => 'ADU (Application Data Unit) MP3 (MPEG audio layer 3) (decoders: mp3adu mp3adufloat )',
          'mp3on4' => 'MP3onMP4 (decoders: mp3on4 mp3on4float )',
          'mp4als' => 'MPEG-4 Audio Lossless Coding (ALS) (decoders: als )',
          'musepack7' => 'Musepack SV7 (decoders: mpc7 )',
          'musepack8' => 'Musepack SV8 (decoders: mpc8 )',
          'nellymoser' => 'Nellymoser Asao',
          'paf_audio' => 'Amazing Studio Packed Animation File Audio',
          'pcm_alaw' => 'PCM A-law / G.711 A-law',
          'pcm_bluray' => 'PCM signed 16|20|24-bit big-endian for Blu-ray media',
          'pcm_dvd' => 'PCM signed 20|24-bit big-endian',
          'pcm_f32be' => 'PCM 32-bit floating point big-endian',
          'pcm_f32le' => 'PCM 32-bit floating point little-endian',
          'pcm_f64be' => 'PCM 64-bit floating point big-endian',
          'pcm_f64le' => 'PCM 64-bit floating point little-endian',
          'pcm_lxf' => 'PCM signed 20-bit little-endian planar',
          'pcm_mulaw' => 'PCM mu-law / G.711 mu-law',
          'pcm_s16be' => 'PCM signed 16-bit big-endian',
          'pcm_s16be_planar' => 'PCM signed 16-bit big-endian planar',
          'pcm_s16le' => 'PCM signed 16-bit little-endian',
          'pcm_s16le_planar' => 'PCM signed 16-bit little-endian planar',
          'pcm_s24be' => 'PCM signed 24-bit big-endian',
          'pcm_s24daud' => 'PCM D-Cinema audio signed 24-bit',
          'pcm_s24le' => 'PCM signed 24-bit little-endian',
          'pcm_s24le_planar' => 'PCM signed 24-bit little-endian planar',
          'pcm_s32be' => 'PCM signed 32-bit big-endian',
          'pcm_s32le' => 'PCM signed 32-bit little-endian',
          'pcm_s32le_planar' => 'PCM signed 32-bit little-endian planar',
          'pcm_s8' => 'PCM signed 8-bit',
          'pcm_s8_planar' => 'PCM signed 8-bit planar',
          'pcm_u16be' => 'PCM unsigned 16-bit big-endian',
          'pcm_u16le' => 'PCM unsigned 16-bit little-endian',
          'pcm_u24be' => 'PCM unsigned 24-bit big-endian',
          'pcm_u24le' => 'PCM unsigned 24-bit little-endian',
          'pcm_u32be' => 'PCM unsigned 32-bit big-endian',
          'pcm_u32le' => 'PCM unsigned 32-bit little-endian',
          'pcm_u8' => 'PCM unsigned 8-bit',
          'pcm_zork' => 'PCM Zork',
          'qcelp' => 'QCELP / PureVoice',
          'qdm2' => 'QDesign Music Codec 2',
          'ra_144' => 'RealAudio 1.0 (14.4K) (decoders: real_144 ) (encoders: real_144 )',
          'ra_288' => 'RealAudio 2.0 (28.8K) (decoders: real_288 )',
          'ralf' => 'RealAudio Lossless',
          'roq_dpcm' => 'DPCM id RoQ',
          's302m' => 'SMPTE 302M',
          'shorten' => 'Shorten',
          'sipr' => 'RealAudio SIPR / ACELP.NET',
          'smackaudio' => 'Smacker audio (decoders: smackaud )',
          'sol_dpcm' => 'DPCM Sol',
          'sonic' => 'Sonic',
          'tak' => "TAK (Tom's lossless Audio Kompressor)",
          'truehd' => 'TrueHD',
          'truespeech' => 'DSP Group TrueSpeech',
          'tta' => 'TTA (True Audio)',
          'twinvq' => 'VQF TwinVQ',
          'vima' => 'LucasArts VIMA audio',
          'vmdaudio' => 'Sierra VMD audio',
          'vorbis' => 'Vorbis (decoders: vorbis libvorbis ) (encoders: vorbis libvorbis )',
          'wavesynth' => 'Wave synthesis pseudo-codec',
          'wavpack' => 'WavPack',
          'westwood_snd1' => 'Westwood Audio (SND1) (decoders: ws_snd1 )',
          'wmalossless' => 'Windows Media Audio Lossless',
          'wmapro' => 'Windows Media Audio 9 Professional',
          'wmav1' => 'Windows Media Audio 1',
          'wmav2' => 'Windows Media Audio 2',
          'wmavoice' => 'Windows Media Audio Voice',
          'xan_dpcm' => 'DPCM Xan',
        ),
        'subtitle' => array(
          'dvb_subtitle' => 'DVB subtitles (decoders: dvbsub ) (encoders: dvbsub )',
          'dvd_subtitle' => 'DVD subtitles (decoders: dvdsub ) (encoders: dvdsub )',
          'hdmv_pgs_subtitle' => 'HDMV Presentation Graphic Stream subtitles (decoders: pgssub )',
          'jacosub' => 'JACOsub subtitle',
          'microdvd' => 'MicroDVD subtitle',
          'mov_text' => 'MOV text',
          'mpl2' => 'MPL2 subtitle',
          'pjs' => 'PJS (Phoenix Japanimation Society) subtitle',
          'realtext' => 'RealText subtitle',
          'sami' => 'SAMI subtitle',
          'srt' => 'SubRip subtitle with embedded timing',
          'ssa' => 'SSA (SubStation Alpha) / ASS (Advanced SSA) subtitle (decoders: ass ) (encoders: ass )',
          'subrip' => 'SubRip subtitle',
          'subviewer' => 'SubViewer subtitle',
          'subviewer1' => 'SubViewer v1 subtitle',
          'text' => 'raw UTF-8 text',
          'vplayer' => 'VPlayer subtitle',
          'webvtt' => 'WebVTT subtitle',
          'xsub' => 'XSUB',
        ),
      ),
      'encode' => array(
        'video' => array(
          'r10k' => 'AJA Kona 10-bit RGB Codec',
          'amv' => 'AMV Video',
          'prores' => 'Apple ProRes (iCodec Pro) (decoders: prores prores_lgpl ) (encoders: prores prores_anatoliy prores_kostya )',
          'asv1' => 'ASUS V1',
          'asv2' => 'ASUS V2',
          'avrp' => 'Avid 1:1 10-bit RGB Packer',
          'avui' => 'Avid Meridien Uncompressed',
          'bmp' => 'BMP (Windows and OS/2 bitmap)',
          'cljr' => 'Cirrus Logic AccuPak',
          'dpx' => 'DPX image',
          'dvvideo' => 'DV (Digital Video)',
          'ffv1' => 'FFmpeg video codec #1',
          'flashsv' => 'Flash Screen Video v1',
          'flashsv2' => 'Flash Screen Video v2',
          'flv1' => 'FLV / Sorenson Spark / Sorenson H.263 (Flash Video) (decoders: flv ) (encoders: flv )',
          'gif' => 'GIF (Graphics Interchange Format)',
          'h261' => 'H.261',
          'h263' => 'H.263 / H.263-1996, H.263+ / H.263-1998 / H.263 version 2',
          'h263p' => 'H.263+ / H.263-1998 / H.263 version 2',
          'h264' => 'H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (encoders: libx264 libx264rgb )',
          'huffyuv' => 'HuffYUV',
          'ffvhuff' => 'Huffyuv FFmpeg variant',
          'roq' => 'id RoQ video (decoders: roqvideo ) (encoders: roqvideo )',
          'jpegls' => 'JPEG-LS',
          'jpeg2000' => 'JPEG 2000 (decoders: j2k ) (encoders: j2k )',
          'zlib' => 'LCL (LossLess Codec Library) ZLIB',
          'ljpeg' => 'Lossless JPEG',
          'msvideo1' => 'Microsoft Video 1',
          'mjpeg' => 'Motion JPEG',
          'mpeg1video' => 'MPEG-1 video',
          'mpeg2video' => 'MPEG-2 video (decoders: mpeg2video mpegvideo )',
          'mpeg4' => 'MPEG-4 part 2 (encoders: mpeg4 libxvid )',
          'msmpeg4v2' => 'MPEG-4 part 2 Microsoft variant version 2',
          'msmpeg4v3' => 'MPEG-4 part 2 Microsoft variant version 3 (decoders: msmpeg4 ) (encoders: msmpeg4 )',
          'a64_multi' => 'Multicolor charset for Commodore 64 (encoders: a64multi )',
          'a64_multi5' => 'Multicolor charset for Commodore 64, extended with 5th color (colram) (encoders: a64multi5 )',
          'vp8' => 'On2 VP8 (decoders: vp8 libvpx ) (encoders: libvpx )',
          'pam' => 'PAM (Portable AnyMap) image',
          'pbm' => 'PBM (Portable BitMap) image',
          'pcx' => 'PC Paintbrush PCX image',
          'pgm' => 'PGM (Portable GrayMap) image',
          'pgmyuv' => 'PGMYUV (Portable GrayMap YUV) image',
          'png' => 'PNG (Portable Network Graphics) image',
          'ppm' => 'PPM (Portable PixelMap) image',
          'qtrle' => 'QuickTime Animation (RLE) video',
          'rawvideo' => 'raw video',
          'rv10' => 'RealVideo 1.0',
          'rv20' => 'RealVideo 1.0',
          'sgi' => 'SGI image',
          'snow' => 'Snow',
          'svq1' => 'Sorenson Vector Quantizer 1 / Sorenson Video 1 / SVQ1',
          'sunrast' => 'Sun Rasterfile image',
          'theora' => 'Theora (encoders: libtheora )',
          'tiff' => 'TIFF image',
          'targa' => 'Truevision Targa image',
          'v210' => 'Uncompressed 4:2:2 10-bit',
          'v410' => 'Uncompressed 4:4:4 10-bit',
          'yuv4' => 'Uncompressed packed 4:2:0',
          'v308' => 'Uncompressed packed 4:4:4',
          'ayuv' => 'Uncompressed packed MS 4:4:4:4',
          'v408' => 'Uncompressed packed QT 4:4:4:4',
          'r210' => 'Uncompressed RGB 10-bit',
          'y41p' => 'Uncompressed YUV 4:1:1 12-bit',
          'utvideo' => 'Ut Video',
          'dnxhd' => 'VC3/DNxHD',
          'wmv1' => 'Windows Media Video 7',
          'wmv2' => 'Windows Media Video 8',
          'xface' => 'X-face image',
          'xbm' => 'XBM (X BitMap) image',
          'xwd' => 'XWD (X Window Dump) image',
          'zmbv' => 'Zip Motion Blocks Video',
        ),
        'audio' => array(
          'aac' => 'AAC (Advanced Audio Coding) (encoders: aac libfaac )',
          'adpcm_ima_qt' => 'ADPCM IMA QuickTime',
          'adpcm_ima_wav' => 'ADPCM IMA WAV',
          'adpcm_ms' => 'ADPCM Microsoft',
          'adpcm_swf' => 'ADPCM Shockwave Flash',
          'adpcm_yamaha' => 'ADPCM Yamaha',
          'alac' => 'ALAC (Apple Lossless Audio Codec)',
          'amr_nb' => 'AMR-NB (Adaptive Multi-Rate NarrowBand) (decoders: amrnb libopencore_amrnb ) (encoders: libopencore_amrnb )',
          'ac3' => 'ATSC A/52A (AC-3) (encoders: ac3 ac3_fixed )',
          'eac3' => 'ATSC A/52B (AC-3, E-AC-3)',
          'dts' => 'DCA (DTS Coherent Acoustics) (decoders: dca ) (encoders: dca )',
          'roq_dpcm' => 'DPCM id RoQ',
          'flac' => 'FLAC (Free Lossless Audio Codec)',
          'adpcm_g722' => 'G.722 ADPCM (decoders: g722 ) (encoders: g722 )',
          'g723_1' => 'G.723.1',
          'adpcm_g726' => 'G.726 ADPCM (decoders: g726 ) (encoders: g726 )',
          'mp2' => 'MP2 (MPEG audio layer 2) (decoders: mp2 mp2float )',
          'mp3' => 'MP3 (MPEG audio layer 3) (decoders: mp3 mp3float ) (encoders: libmp3lame )',
          'nellymoser' => 'Nellymoser Asao',
          'pcm_f32be' => 'PCM 32-bit floating point big-endian',
          'pcm_f32le' => 'PCM 32-bit floating point little-endian',
          'pcm_f64be' => 'PCM 64-bit floating point big-endian',
          'pcm_f64le' => 'PCM 64-bit floating point little-endian',
          'pcm_alaw' => 'PCM A-law / G.711 A-law',
          'pcm_s24daud' => 'PCM D-Cinema audio signed 24-bit',
          'pcm_mulaw' => 'PCM mu-law / G.711 mu-law',
          'pcm_s8' => 'PCM signed 8-bit',
          'pcm_s8_planar' => 'PCM signed 8-bit planar',
          'pcm_s16be' => 'PCM signed 16-bit big-endian',
          'pcm_s16be_planar' => 'PCM signed 16-bit big-endian planar',
          'pcm_s16le' => 'PCM signed 16-bit little-endian',
          'pcm_s16le_planar' => 'PCM signed 16-bit little-endian planar',
          'pcm_s24be' => 'PCM signed 24-bit big-endian',
          'pcm_s24le' => 'PCM signed 24-bit little-endian',
          'pcm_s24le_planar' => 'PCM signed 24-bit little-endian planar',
          'pcm_s32be' => 'PCM signed 32-bit big-endian',
          'pcm_s32le' => 'PCM signed 32-bit little-endian',
          'pcm_s32le_planar' => 'PCM signed 32-bit little-endian planar',
          'pcm_u8' => 'PCM unsigned 8-bit',
          'pcm_u16be' => 'PCM unsigned 16-bit big-endian',
          'pcm_u16le' => 'PCM unsigned 16-bit little-endian',
          'pcm_u24be' => 'PCM unsigned 24-bit big-endian',
          'pcm_u24le' => 'PCM unsigned 24-bit little-endian',
          'pcm_u32be' => 'PCM unsigned 32-bit big-endian',
          'pcm_u32le' => 'PCM unsigned 32-bit little-endian',
          'ra_144' => 'RealAudio 1.0 (14.4K) (decoders: real_144 ) (encoders: real_144 )',
          'comfortnoise' => 'RFC 3389 Comfort Noise',
          'adpcm_adx' => 'SEGA CRI ADX ADPCM',
          'sonic' => 'Sonic',
          'sonicls' => 'Sonic lossless',
          'vorbis' => 'Vorbis (decoders: vorbis libvorbis ) (encoders: vorbis libvorbis )',
          'wmav1' => 'Windows Media Audio 1',
          'wmav2' => 'Windows Media Audio 2',
        ),
        'subtitle' => array(
          'dvb_subtitle' => 'DVB subtitles (decoders: dvbsub ) (encoders: dvbsub )',
          'dvd_subtitle' => 'DVD subtitles (decoders: dvdsub ) (encoders: dvdsub )',
          'mov_text' => 'MOV text',
          'srt' => 'SubRip subtitle with embedded timing',
          'ssa' => 'SSA (SubStation Alpha) / ASS (Advanced SSA) subtitle (decoders: ass ) (encoders: ass )',
          'subrip' => 'SubRip subtitle',
          'xsub' => 'XSUB',
        ),
      ),
    );
  }

  /**
   * Get available output file formats from the transcoder.
   */
  public function getAvailableFormats($type = FALSE) {
    $info = array(
      'formats' => array(
        '3g2' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => '3GP2 (3GPP2 file format)',
        ),
        '3gp' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => '3GP (3GPP file format)',
        ),
        '4xm' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => '4X Technologies',
        ),
        'a64' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'a64 - video for Commodore 64',
        ),
        'aac' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw ADTS AAC (Advanced Audio Coding)',
        ),
        'ac3' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw AC-3',
        ),
        'act' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'ACT Voice file format',
        ),
        'adf' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Artworx Data Format',
        ),
        'adts' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'ADTS AAC (Advanced Audio Coding)',
        ),
        'adx' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'CRI ADX',
        ),
        'aea' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'MD STUDIO audio',
        ),
        'afc' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'AFC',
        ),
        'aiff' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Audio IFF',
        ),
        'alaw' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM A-law',
        ),
        'amr' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => '3GPP AMR',
        ),
        'anm' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Deluxe Paint Animation',
        ),
        'apc' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'CRYO APC',
        ),
        'ape' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => "Monkey's Audio\r",
        ),
        'aqtitle' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'AQTitle subtitles',
        ),
        'asf' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'ASF (Advanced / Active Streaming Format)',
        ),
        'asf_stream' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'ASF (Advanced / Active Streaming Format)',
        ),
        'ass' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'SSA (SubStation Alpha) subtitle',
        ),
        'ast' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'AST (Audio Stream)',
        ),
        'au' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Sun AU',
        ),
        'avi' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'AVI (Audio Video Interleaved)',
        ),
        'avm2' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'SWF (ShockWave Flash) (AVM2)',
        ),
        'avr' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'AVR (Audio Visual Research)',
        ),
        'avs' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'AVS',
        ),
        'bethsoftvid' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Bethesda Softworks VID',
        ),
        'bfi' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Brute Force & Ignorance',
        ),
        'bin' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Binary text',
        ),
        'bink' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Bink',
        ),
        'bit' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'G.729 BIT file format',
        ),
        'bmv' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Discworld II BMV',
        ),
        'brstm' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'BRSTM (Binary Revolution Stream)',
        ),
        'c93' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Interplay C93',
        ),
        'caf' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Apple CAF (Core Audio Format)',
        ),
        'cavsvideo' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw Chinese AVS (Audio Video Standard) video',
        ),
        'cdg' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'CD Graphics',
        ),
        'cdxl' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Commodore CDXL video',
        ),
        'concat' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Virtual concatenation script',
        ),
        'crc' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'CRC testing',
        ),
        'daud' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'D-Cinema audio',
        ),
        'dfa' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Chronomaster DFA',
        ),
        'dirac' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw Dirac',
        ),
        'dnxhd' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw DNxHD (SMPTE VC-3)',
        ),
        'dsicin' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Delphine Software International CIN',
        ),
        'dts' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw DTS',
        ),
        'dtshd' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw DTS-HD',
        ),
        'dv' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'DV (Digital Video)',
        ),
        'dv1394' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'DV1394 A/V grab',
        ),
        'dvd' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MPEG-2 PS (DVD VOB)',
        ),
        'dxa' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'DXA',
        ),
        'ea' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Electronic Arts Multimedia',
        ),
        'ea_cdata' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Electronic Arts cdata',
        ),
        'eac3' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw E-AC-3',
        ),
        'epaf' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Ensoniq Paris Audio File',
        ),
        'f32be' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM 32-bit floating-point big-endian',
        ),
        'f32le' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM 32-bit floating-point little-endian',
        ),
        'f4v' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'F4V Adobe Flash Video',
        ),
        'f64be' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM 64-bit floating-point big-endian',
        ),
        'f64le' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM 64-bit floating-point little-endian',
        ),
        'fbdev' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Linux framebuffer',
        ),
        'ffm' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'FFM (FFserver live feed)',
        ),
        'ffmetadata' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'FFmpeg metadata in text',
        ),
        'film_cpk' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Sega FILM / CPK',
        ),
        'filmstrip' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Adobe Filmstrip',
        ),
        'flac' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw FLAC',
        ),
        'flic' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'FLI/FLC/FLX animation',
        ),
        'flv' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'FLV (Flash Video)',
        ),
        'framecrc' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'framecrc testing',
        ),
        'framemd5' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'Per-frame MD5 testing',
        ),
        'frm' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Megalux Frame',
        ),
        'g722' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw G.722',
        ),
        'g723_1' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw G.723.1',
        ),
        'g729' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'G.729 raw format demuxer',
        ),
        'gif' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'GIF Animation',
        ),
        'gsm' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw GSM',
        ),
        'gxf' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'GXF (General eXchange Format)',
        ),
        'h261' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw H.261',
        ),
        'h263' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw H.263',
        ),
        'h264' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw H.264 video',
        ),
        'hls' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'Apple HTTP Live Streaming',
        ),
        'hls,applehttp' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Apple HTTP Live Streaming',
        ),
        'ico' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Microsoft Windows ICO',
        ),
        'idcin' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'id Cinematic',
        ),
        'idf' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'iCE Draw File',
        ),
        'iff' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'IFF (Interchange File Format)',
        ),
        'ilbc' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'iLBC storage',
        ),
        'image2' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'image2 sequence',
        ),
        'image2pipe' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'piped image2 sequence',
        ),
        'ingenient' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw Ingenient MJPEG',
        ),
        'ipmovie' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Interplay MVE',
        ),
        'ipod' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'iPod H.264 MP4 (MPEG-4 Part 14)',
        ),
        'ircam' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Berkeley/IRCAM/CARL Sound Format',
        ),
        'ismv' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'ISMV/ISMA (Smooth Streaming)',
        ),
        'iss' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Funcom ISS',
        ),
        'iv8' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'IndigoVision 8000 video',
        ),
        'ivf' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'On2 IVF',
        ),
        'jacosub' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'JACOsub subtitle format',
        ),
        'jv' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Bitmap Brothers JV',
        ),
        'latm' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'LOAS/LATM',
        ),
        'lavfi' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Libavfilter virtual input device',
        ),
        'lmlm4' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw lmlm4',
        ),
        'loas' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'LOAS AudioSyncStream',
        ),
        'lvf' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'LVF',
        ),
        'lxf' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'VR native stream (LXF)',
        ),
        'm4v' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw MPEG-4 video',
        ),
        'matroska' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'Matroska',
        ),
        'matroska,webm' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Matroska / WebM',
        ),
        'md5' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MD5 testing',
        ),
        'mgsts' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Metal Gear Solid: The Twin Snakes',
        ),
        'microdvd' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'MicroDVD subtitle format',
        ),
        'mjpeg' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw MJPEG video',
        ),
        'mlp' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw MLP',
        ),
        'mm' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'American Laser Games MM',
        ),
        'mmf' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Yamaha SMAF',
        ),
        'mov' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'QuickTime / MOV',
        ),
        'mp2' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MP2 (MPEG audio layer 2)',
        ),
        'mp3' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'MP3 (MPEG audio layer 3)',
        ),
        'mp4' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MP4 (MPEG-4 Part 14)',
        ),
        'mpc' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Musepack',
        ),
        'mpc8' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Musepack SV8',
        ),
        'mpeg' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'MPEG-1 Systems / MPEG program stream',
        ),
        'mpeg1video' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'raw MPEG-1 video',
        ),
        'mpeg2video' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'raw MPEG-2 video',
        ),
        'mpegts' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'MPEG-TS (MPEG-2 Transport Stream)',
        ),
        'mpegtsraw' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw MPEG-TS (MPEG-2 Transport Stream)',
        ),
        'mpegvideo' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw MPEG video',
        ),
        'mpjpeg' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MIME multipart JPEG',
        ),
        'mpl2' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'MPL2 subtitles',
        ),
        'mpsub' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'MPlayer subtitles',
        ),
        'msnwctcp' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'MSN TCP Webcam stream',
        ),
        'mtv' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'MTV',
        ),
        'mulaw' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM mu-law',
        ),
        'mv' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Silicon Graphics Movie',
        ),
        'mvi' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Motion Pixels MVI',
        ),
        'mxf' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'MXF (Material eXchange Format)',
        ),
        'mxf_d10' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MXF (Material eXchange Format) D-10 Mapping',
        ),
        'mxg' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'MxPEG clip',
        ),
        'nc' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'NC camera feed',
        ),
        'nistsphere' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'NIST SPeech HEader REsources',
        ),
        'nsv' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Nullsoft Streaming Video',
        ),
        'null' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'raw null video',
        ),
        'nut' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'NUT',
        ),
        'nuv' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'NuppelVideo',
        ),
        'ogg' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Ogg',
        ),
        'oma' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Sony OpenMG audio',
        ),
        'oss' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'OSS (Open Sound System) playback',
        ),
        'paf' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Amazing Studio Packed Animation File',
        ),
        'pjs' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'PJS (Phoenix Japanimation Society) subtitles',
        ),
        'pmp' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Playstation Portable PMP',
        ),
        'psp' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'PSP MP4 (MPEG-4 Part 14)',
        ),
        'psxstr' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Sony Playstation STR',
        ),
        'pva' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'TechnoTrend PVA',
        ),
        'pvf' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'PVF (Portable Voice Format)',
        ),
        'qcp' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'QCP',
        ),
        'r3d' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'REDCODE R3D',
        ),
        'rawvideo' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw video',
        ),
        'rcv' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'VC-1 test bitstream',
        ),
        'realtext' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'RealText subtitle format',
        ),
        'rl2' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'RL2',
        ),
        'rm' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'RealMedia',
        ),
        'roq' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw id RoQ',
        ),
        'rpl' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'RPL / ARMovie',
        ),
        'rso' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Lego Mindstorms RSO',
        ),
        'rtp' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'RTP output',
        ),
        'rtsp' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'RTSP output',
        ),
        's16be' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM signed 16-bit big-endian',
        ),
        's16le' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM signed 16-bit little-endian',
        ),
        's24be' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM signed 24-bit big-endian',
        ),
        's24le' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM signed 24-bit little-endian',
        ),
        's32be' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM signed 32-bit big-endian',
        ),
        's32le' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM signed 32-bit little-endian',
        ),
        's8' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM signed 8-bit',
        ),
        'sami' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'SAMI subtitle format',
        ),
        'sap' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'SAP output',
        ),
        'sbg' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'SBaGen binaural beats script',
        ),
        'sdp' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'SDP',
        ),
        'segment' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'segment',
        ),
        'shn' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw Shorten',
        ),
        'siff' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Beam Software SIFF',
        ),
        'smjpeg' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Loki SDL MJPEG',
        ),
        'smk' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Smacker',
        ),
        'smush' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'LucasArts Smush',
        ),
        'sol' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Sierra SOL',
        ),
        'sox' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'SoX native',
        ),
        'spdif' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'IEC 61937 (used on S/PDIF - IEC958)',
        ),
        'srt' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'SubRip subtitle',
        ),
        'subviewer' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'SubViewer subtitle format',
        ),
        'subviewer1' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'SubViewer v1 subtitle format',
        ),
        'svcd' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MPEG-2 PS (SVCD)',
        ),
        'swf' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'SWF (ShockWave Flash)',
        ),
        'tak' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw TAK',
        ),
        'tedcaptions' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'TED Talks captions',
        ),
        'thp' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'THP',
        ),
        'tiertexseq' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Tiertex Limited SEQ',
        ),
        'tmv' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => '8088flex TMV',
        ),
        'truehd' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'raw TrueHD',
        ),
        'tta' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'TTA (True Audio)',
        ),
        'tty' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Tele-typewriter',
        ),
        'txd' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Renderware TeXture Dictionary',
        ),
        'u16be' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM unsigned 16-bit big-endian',
        ),
        'u16le' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM unsigned 16-bit little-endian',
        ),
        'u24be' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM unsigned 24-bit big-endian',
        ),
        'u24le' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM unsigned 24-bit little-endian',
        ),
        'u32be' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM unsigned 32-bit big-endian',
        ),
        'u32le' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM unsigned 32-bit little-endian',
        ),
        'u8' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'PCM unsigned 8-bit',
        ),
        'vc1' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'raw VC-1',
        ),
        'vc1test' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'VC-1 test bitstream',
        ),
        'vcd' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MPEG-1 Systems / MPEG program stream (VCD)',
        ),
        'vivo' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Vivo',
        ),
        'vmd' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Sierra VMD',
        ),
        'vob' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'MPEG-2 PS (VOB)',
        ),
        'vobsub' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'VobSub subtitle format',
        ),
        'voc' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Creative Voice',
        ),
        'vplayer' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'VPlayer subtitles',
        ),
        'vqf' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Nippon Telegraph and Telephone Corporation (NTT) TwinVQ',
        ),
        'w64' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Sony Wave64',
        ),
        'wav' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'WAV / WAVE (Waveform Audio)',
        ),
        'wc3movie' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Wing Commander III movie',
        ),
        'webm' => array(
          'mux' => TRUE,
          'demux' => FALSE,
          'fullname' => 'WebM',
        ),
        'webvtt' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'WebVTT subtitle',
        ),
        'wsaud' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Westwood Studios audio',
        ),
        'wsvqa' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Westwood Studios VQA',
        ),
        'wtv' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'Windows Television (WTV)',
        ),
        'wv' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'WavPack',
        ),
        'xa' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Maxis XA',
        ),
        'xbin' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'eXtended BINary text (XBIN)',
        ),
        'xmv' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Microsoft XMV',
        ),
        'xwma' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Microsoft xWMA',
        ),
        'yop' => array(
          'mux' => FALSE,
          'demux' => TRUE,
          'fullname' => 'Psygnosis YOP',
        ),
        'yuv4mpegpipe' => array(
          'mux' => TRUE,
          'demux' => TRUE,
          'fullname' => 'YUV4MPEG pipe',
        ),
      ),
    );
    $formats = array();
    switch ($type) {
      case FALSE:
        return array_keys($info['formats']);
      case 'both' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['mux'] === TRUE && $data['demux'] === TRUE) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
      case 'muxing' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['mux'] === TRUE) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
      case 'demuxing' :
        foreach ($info['formats'] as $id => $data) {
          if ($data['demux'] === TRUE) {
            $formats[$id] = $data['fullname'];
          }
        }
        break;
    }

    if (isset($formats['ogg']) && !isset($formats['ogv'])) {
      $formats['ogv'] = $formats['ogg'];
      unset($formats['ogg']);
      asort($formats);
    }

    return $formats;
  }

  /**
   * Returns a list of all supported pixel formats.
   *
   * Returns NULL when pixel formats are not configurable for this transcoder.
   */
  public function getPixelFormats() {
    return array(
      'yuv420p',
      'yuyv422',
      'rgb24',
      'bgr24',
      'yuv422p',
      'yuv444p',
      'yuv410p',
      'yuv411p',
      'gray',
      'monow',
      'monob',
      'pal8',
      'yuvj420p',
      'yuvj422p',
      'yuvj444p',
      'xvmcmc',
      'xvmcidct',
      'uyvy422',
      'uyyvyy411',
      'bgr8',
      'bgr4',
      'bgr4_byte',
      'rgb8',
      'rgb4',
      'rgb4_byte',
      'nv12',
      'nv21',
      'argb',
      'rgba',
      'abgr',
      'bgra',
      'gray16be',
      'gray16le',
      'yuv440p',
      'yuvj440p',
      'yuva420p',
      'vdpau_h264',
      'vdpau_mpeg1',
      'vdpau_mpeg2',
      'vdpau_wmv3',
      'vdpau_vc1',
      'rgb48be',
      'rgb48le',
      'rgb565be',
      'rgb565le',
      'rgb555be',
      'rgb555le',
      'bgr565be',
      'bgr565le',
      'bgr555be',
      'bgr555le',
      'vaapi_moco',
      'vaapi_idct',
      'vaapi_vld',
      'yuv420p16le',
      'yuv420p16be',
      'yuv422p16le',
      'yuv422p16be',
      'yuv444p16le',
      'yuv444p16be',
      'vdpau_mpeg4',
      'dxva2_vld',
      'rgb444le',
      'rgb444be',
      'bgr444le',
      'bgr444be',
      'gray8a',
      'bgr48be',
      'bgr48le',
      'yuv420p9be',
      'yuv420p9le',
      'yuv420p10be',
      'yuv420p10le',
      'yuv422p10be',
      'yuv422p10le',
      'yuv444p9be',
      'yuv444p9le',
      'yuv444p10be',
      'yuv444p10le',
      'yuv422p9be',
      'yuv422p9le',
      'vda_vld',
      'gbrp',
      'gbrp9be',
      'gbrp9le',
      'gbrp10be',
      'gbrp10le',
      'gbrp16be',
      'gbrp16le',
      'yuva420p9be',
      'yuva420p9le',
      'yuva422p9be',
      'yuva422p9le',
      'yuva444p9be',
      'yuva444p9le',
      'yuva420p10be',
      'yuva420p10le',
      'yuva422p10be',
      'yuva422p10le',
      'yuva444p10be',
      'yuva444p10le',
      'yuva420p16be',
      'yuva420p16le',
      'yuva422p16be',
      'yuva422p16le',
      'yuva444p16be',
      'yuva444p16le',
      'vdpau',
      'rgba64be',
      'rgba64le',
      'bgra64be',
      'bgra64le',
      '0rgb',
      'rgb0',
      '0bgr',
      'bgr0',
      'yuva444p',
      'yuva422p',
      'yuv420p12be',
      'yuv420p12le',
      'yuv420p14be',
      'yuv420p14le',
      'yuv422p12be',
      'yuv422p12le',
      'yuv422p14be',
      'yuv422p14le',
      'yuv444p12be',
      'yuv444p12le',
      'yuv444p14be',
      'yuv444p14le',
      'gbrp12be',
      'gbrp12le',
      'gbrp14be',
      'gbrp14le',
    );
  }

  /**
   * Admin settings form for the transcoder
   */
  public function adminSettings() {
    $form = array();

    $form['transcoderstub'] = array(
      '#type' => 'fieldset',
      '#title' => $this->getName(),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE,
      '#states' => array(
        'visible' => array(
          ':input[name=video_convertor]' => array('value' => 'TranscoderAbstractionFactoryStub'),
        ),
      ),
    );

    // Thumbnail count.
    $form['transcoderstub']['video_thumbnail_count_stub'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of thumbnails'),
      '#description' => t('Number of thumbnails to fake per video.'),
      '#default_value' => variable_get('video_thumbnail_count', 5),
      '#size' => 5,
    );

    // Transcoding delay.
    $form['transcoderstub']['transcoderstub_delay'] = array(
      '#type' => 'textfield',
      '#title' => t('Transcoding delay.'),
      '#description' => t('How many seconds each simulated transcoding should take.'),
      '#default_value' => variable_get('transcoderstub_delay', 0),
    );

    // Debug logging.
    $form['transcoderstub']['transcoderstub_debug_enabled'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable debug logging.'),
      '#description' => t('Log debug information to watchdog.'),
      '#default_value' => variable_get('transcoderstub_debug_enabled', FALSE),
    );
    return $form;
  }

  /**
   * Validate admin settings. This will call when Drupal admin settings validate.
   */
  public function adminSettingsValidate($form, &$form_state) {
    $thumbs = $form_state['values']['video_thumbnail_count_stub'];
    if (!is_numeric($thumbs) || intval($thumbs) != floatval($thumbs) || $thumbs < 1 || $thumbs > 100000) {
      form_set_error('video_thumbnail_count_stub', t('Number of thumbnails must be a whole number between 1 and 100000'));
    }
    $delay = $form_state['values']['transcoderstub_delay'];
    if (!is_numeric($delay) || intval($delay) != floatval($delay) || $delay < 0 || $delay > 604800) {
      form_set_error('transcoderstub_delay', t('Delay must be a whole number between 0 and 604800 (1 week).'));
    }
    // Workaround for using of the same variable in FFmpeg.
    $form_state['values']['video_thumbnail_count'] = $form_state['values']['video_thumbnail_count_stub'];
    unset($form_state['values']['video_thumbnail_count_stub']);
  }

  /**
   * @param array $options
   * @return bool|void
   */
  public function setOptions(array $options) {
    if ($this->debug) {
      watchdog('transcoderstub', '@method called. Options: <pre>@data</pre>', array('@method' => __METHOD__, '@data' => print_r($options, TRUE)), WATCHDOG_DEBUG);
    }
    parent::setOptions($options);
    return TRUE;
  }
}
