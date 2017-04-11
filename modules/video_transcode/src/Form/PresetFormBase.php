<?php

namespace Drupal\video_transcode\Form;

use Drupal\Core\Entity\EntityForm;
use Drupal\Core\Entity\Query\QueryFactory;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Url;

/**
 * Class PresetFormBase.
 *
 * Typically, we need to build the same form for both adding a new entity,
 * and editing an existing entity. Instead of duplicating our form code,
 * we create a base class. Drupal never routes to this class directly,
 * but instead through the child classes of PresetAddForm and PresetFormBase.
 *
 * @package Drupal\video_transcode\Form
 *
 * @ingroup video_transcode
 */
class PresetFormBase extends EntityForm {

  /**
   * @var \Drupal\Core\Entity\Query\QueryFactory
   */
  protected $entityQueryFactory;

  /**
   * Construct the PresetFormBase.
   *
   * For simple entity forms, there's no need for a constructor. Our preset form
   * base, however, requires an entity query factory to be injected into it
   * from the container. We later use this query factory to build an entity
   * query for the exists() method.
   *
   * @param \Drupal\Core\Entity\Query\QueryFactory $query_factory
   *   An entity query factory for the preset entity type.
   */
  public function __construct(QueryFactory $query_factory) {
    $this->entityQueryFactory = $query_factory;
  }

  /**
   * Factory method for PresetFormBase.
   *
   * When Drupal builds this class it does not call the constructor directly.
   * Instead, it relies on this method to build the new object. Why? The class
   * constructor may take multiple arguments that are unknown to Drupal. The
   * create() method always takes one parameter -- the container. The purpose
   * of the create() method is twofold: It provides a standard way for Drupal
   * to construct the object, meanwhile it provides you a place to get needed
   * constructor parameters from the container.
   *
   * In this case, we ask the container for an entity query factory. We then
   * pass the factory to our class as a constructor parameter.
   */
  public static function create(ContainerInterface $container) {
    return new static($container->get('entity.query'));
  }

  /**
   * Overrides Drupal\Core\Entity\EntityFormController::form().
   *
   * Builds the entity add/edit form.
   *
   * @param array $form
   *   An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   An associative array containing the current state of the form.
   *
   * @return array
   *   An associative array containing the preset add/edit form.
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // Get anything we need from the base class.
    $form = parent::buildForm($form, $form_state);

    // Drupal provides the entity to us as a class variable. If this is an
    // existing entity, it will be populated with existing values as class
    // variables. If this is a new entity, it will be a new object with the
    // class of our entity. Drupal knows which class to call from the
    // annotation on our Preset class.
    $preset = $this->entity;
    // Build the form.
    $form['label'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Label'),
      '#maxlength' => 255,
      '#default_value' => $preset->label(),
      '#required' => TRUE,
      '#weight' => -10
    );
    $form['id'] = array(
      '#type' => 'machine_name',
      '#title' => $this->t('Machine name'),
      '#default_value' => $preset->id(),
      '#machine_name' => array(
        'exists' => array($this, 'exists'),
        'replace_pattern' => '([^a-z0-9_]+)|(^custom$)',
        'error' => 'The machine-readable name must be unique, and can only contain lowercase letters, numbers, and underscores. Additionally, it can not be the reserved word "custom".',
      ),
      '#disabled' => !$preset->isNew(),
      '#weight' => -10
    );
    // Container/File format
    $form['video_extension'] = array(
      '#type' => 'select',
      '#title' => $this->t('Output extension/Container'),
      '#description' => $this->t('Extension of the output. Use mp4 for H.264 content. Use fmp4 for Smooth Streaming or MPEG-DASH content. Use ts for HLS content. Use webm for vp8/vp9 downloaded content. Use mp3 for mp3 audio. Use ogg for vorbis audio. Use oga for flac audio. Use flac for flac audio. Use mpg for mpeg2 content. Use flv for Flash content. Use gif for Animated Gif videos. Use mxf for XDCAM content. Use wav for pcm audio.'),
      '#options' => ['3gp' => '3GP', 'aac' => 'AAC', 'ac3' => 'AC3', 'ec3' => 'EC3', 'flv' => 'FLV', 'm4f' => 'M4F', 'mj2' => 'MJ2', 'mkv' => 'MKV', 'mp3' => 'MP3', 'mp4' => 'MP4', 'mxf' => 'MXF', 'ogg' => 'OGG', 'oga' => 'OGA', 'ts' => 'TS', 'webm' => 'WEBM', 'wmv' => 'WMV', 'fmp4'=>'FMP4', 'flac'=> 'FLAC', 'mpg' => 'MPG', 'gif' => 'GIF'],
      '#default_value' => !empty($preset->video_extension) ? $preset->video_extension :  'mp4',
      '#required' => TRUE,
      '#weight' => -9
    );
    
    // video settings
    $form['settings']['video'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Video settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );

    $form['settings']['video']['video_codec'] = array(
      '#type' => 'select',
      '#title' => $this->t('Video codec'),
      '#description' => $this->t('The video codec used in the video file can affect the ability to play the video on certain devices.'),
      '#options' => ['h264' => 'H246', 'hevc' => 'HEVC', 'jp2' => 'JP2', 'mpeg4' => 'MPEG4', 'theora' => 'Theora', 'vp6' => 'VP6', 'vp8' => 'VP8', 'vp9' => 'VP9', 'wmv' => 'WMV'],
      '#default_value' => !empty($preset->video_codec) ? $preset->video_codec :  'h264',
      '#required' => TRUE,
    );

    $form['settings']['video']['video_quality'] = array(
      '#type' => 'select',
      '#title' => $this->t('Video quality'),
      '#description' => $this->t('A target video quality. Affects bitrate and file size.'),
      '#options' => array(
        'none' => $this->t('None'),
        1 => '1 - Poor quality (smaller file)',
        2 => '2',
        3 => '3' . ' (' . $this->t('default') . ')',
        4 => '4',
        5 => '5 - High quality (larger file)'
      ),
      '#default_value' => (!empty($preset->video_quality)) ? $preset->video_quality : 3,
    );
    
    $form['settings']['video']['video_speed'] = array(
      '#type' => 'select',
      '#title' => $this->t('Video speed'),
      '#description' => $this->t('Speed of encoding. Affects compression.'),
      '#options' => array(
        'none' => $this->t('None'),
        1 => '1 - Slow (better compression)',
        2 => '2',
        3 => '3' . ' (' . $this->t('default') . ')',
        4 => '4',
        5 => '5 - Fast (worse compression)'
      ),
      '#default_value' => (!empty($preset->video_speed)) ? $preset->video_speed : 3,
    );
    
    $form['settings']['video']['wxh'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Dimensions'),
      '#description' => $this->t('The resolution of the output file, expressed as WxH, like 640×480 or 1280×720.'),
      '#default_value' => !empty($preset->wxh) ? $preset->wxh : '640x360',
    );
    
    $form['settings']['video']['video_aspectmode'] = array(
      '#type' => 'select',
      '#title' => $this->t('Aspect mode'),
      '#description' => $this->t('What to do when aspect ratio of input file does not match the target width/height aspect ratio.'),
      '#options' => array(
        'preserve' => $this->t('Preserve aspect ratio') . ' (' . $this->t('default') . ')',
        'crop' => $this->t('Crop to fit output aspect ratio'),
        'pad' => $this->t('Pad (letterbox) to fit output aspect ratio'),
        'stretch' => $this->t('Stretch (distort) to output aspect ratio'),
      ),
      '#default_value' => (!empty($preset->video_aspectmode)) ? $preset->video_aspectmode : 'preserve',
    );
    
    $form['settings']['video']['video_upscale'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Upscale'),
      '#description' => $this->t('If the input file is smaller than the target output, should the file be upscaled to the target size?'),
      '#default_value' => !empty($preset->video_upscale) ? $preset->video_upscale : ''
    );
    
    // audio settings
    $form['settings']['audio'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Audio settings'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );

    $form['settings']['audio']['audio_codec'] = array(
      '#type' => 'select',
      '#title' => $this->t('Audio codec'),
      '#description' => $this->t('The audio codec to be used.'),
      '#options' => ['aac'=>'AAC', 'ac3'=>'AC3', 'amr'=>'AMR', 'eac3'=>'EAC3', 'mp3'=>'MP3', 'pcm'=>'PCM', 'vorbis'=>'Vorbis', 'wma'=>'WMA'],
      '#required' => TRUE,
      '#default_value' => (!empty($preset->audio_codec)) ? $preset->audio_codec : 'mp3',
    );
    
    $form['settings']['audio']['audio_quality'] = array(
      '#type' => 'select',
      '#title' => $this->t('Audio quality'),
      '#description' => $this->t('A target audio quality. Affects bitrate and file size.'),
      '#options' => array(
        '' => $this->t('None'),
        1 => '1 - Poor quality (smaller file)',
        2 => '2',
        3 => '3' . ' (' . $this->t('default') . ')',
        4 => '4',
        5 => '5 - High quality (larger file)'
      ),
      '#default_value' => (!empty($preset->audio_quality)) ? $preset->audio_quality : 3,
    );
    
    // advanced video settings
    $form['settings']['adv_video'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Advanced video settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE
    );
    $form['settings']['adv_video']['deinterlace'] = array(
      '#type' => 'select',
      '#title' => $this->t('Deinterlace'),
      '#description' => $this->t('Note that detect mode will auto-detect and deinterlace interlaced content.'),
      '#options' => array(
        'detect' => 'Detect' . ' (' . $this->t('default') . ')',
        'on' => 'On (reduces quality of non-interlaced content)',
        'off' => 'Off'
      ),
      '#default_value' => (!empty($preset->deinterlace)) ? $preset->deinterlace : 'detect'
    );
    $form['settings']['adv_video']['max_frame_rate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Maximum frame rate'),
      '#description' => $this->t('A maximum frame rate cap (in frames per second).'),
      '#default_value' => !empty($preset->max_frame_rate) ? $preset->max_frame_rate : ''
    );
    $form['settings']['adv_video']['frame_rate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Frame rate'),
      '#description' => $this->t('Force a specific output frame rate (in frames per second). For best quality, do not use this setting.'),
      '#default_value' => !empty($preset->frame_rate) ? $preset->frame_rate : ''
    );
    $form['settings']['adv_video']['keyframe_interval'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Key frame interval'),
      '#description' => $this->t('By default, a keyframe will be created at most every 250 frames. Specifying a different keyframe interval will allow you to create more or fewer keyframes in your video. A greater number of keyframes will increase the size of your output file, but will allow for more precise scrubbing in most players. Keyframe interval should be specified as a positive integer. For example, a value of 100 will create a keyframe every 100 frames.'),
      '#default_value' => !empty($preset->keyframe_interval) ? $preset->keyframe_interval : ''
    );
    $form['settings']['adv_video']['video_bitrate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Video bitrate'),
      '#description' => $this->t('A target bitrate in kbps. Not necessary if you select a Video Quality setting, unless you want to target a specific bitrate.'),
      '#default_value' => !empty($preset->video_bitrate) ? $preset->video_bitrate : '',
    );
    $form['settings']['adv_video']['bitrate_cap'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Bitrate cap'),
      '#description' => $this->t('A bitrate cap in kbps, used for streaming servers.'),
      '#default_value' => !empty($preset->bitrate_cap) ? $preset->bitrate_cap : ''
    );
    $form['settings']['adv_video']['buffer_size'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Buffer size'),
      '#description' => $this->t('The buffer size for the bitrate cap in kbps.'),
      '#default_value' => !empty($preset->buffer_size) ? $preset->buffer_size : ''
    );
    $form['settings']['adv_video']['one_pass'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Force one-pass encoding'),
      '#default_value' => !empty($preset->one_pass) ? $preset->one_pass : ''
    );
    $form['settings']['adv_video']['skip_video'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Skip video'),
      '#description' => $this->t('The video track will be omitted from the output. You can still specify a video format, however, no video track will be present in the resulting file.'),
      '#default_value' => !empty($preset->skip_video) ? $preset->skip_video : ''
    );
    
    $reference_frames = ['auto' => 'auto'] + range(0, 16);
    $form['settings']['adv_video']['reference_frames'] = array(
      '#type' => 'select',
      '#title' => $this->t('Video reference frames'),
      '#description' => $this->t('Number of reference frames to use. More reference frames result in slightly higher compression quality, but increased decoding complexity. In practice, going above 5 rarely has much benefit. Determined partly by speed as well as video_codec_profile. Set to "auto" to allow our speed setting to naturally choose this number. We default to 3 as a good compromise of compression and decoding complexity. Use 1 for video created for legacy iPod or first-generation iPhone video, or for other technically-limited decoders.'),
      '#options' => $reference_frames,
      '#default_value' => !empty($preset->reference_frames) ? $preset->reference_frames : 3,
    );

    $profiles = array('' => $this->t('None'), 'baseline' => 'Baseline', 'main' => 'Main', 'high' => 'High');
    $form['settings']['adv_video']['h264_profile'] = array(
      '#type' => 'select',
      '#title' => $this->t('H.264 profile'),
      '#description' => $this->t('Use Baseline for maximum compatibility with players. Select @optionnamenone when this is not an H.264 preset or when setting the profile causes errors.', array('@optionnamenone' => $this->t('None'))),
      '#options' => $profiles,
      '#default_value' => !empty($preset->h264_profile) ? $preset->h264_profile : '1',
    );
    
    $codec_levels = ['1'=>'1', '1b'=>'1b', '1.1'=>'1.1', '1.2'=>'1.2', '1.3'=>'1.3', '2'=>'2', '2.1'=>'2.1', '2.2'=>'2.2', '3'=>'3', '3.1'=>'3.1', '3.2'=>'3.2', '4'=>'4', '4.1'=>'4.1', '4.2'=>'4.2', '5'=>'5', '5.1'=>'5.1', '5.2'=>'5.2', '6'=>'6', '6.1'=>'6.1', '6.2'=>'6.2'];
    $form['settings']['adv_video']['codec_level'] = array(
      '#type' => 'select',
      '#title' => $this->t('Video codec level'),
      '#description' => $this->t('Constrains bitrate, macroblocks (H.264) or bitrate, coding tree units (HEVC). Primarily used for device compatibility. For example, the iPhone supports H.264 Level 3, which means that a video’s decoder_bitrate_cap can’t exceed 10,000kbps. Typically, you should only change this setting if you’re targeting a specific device that requires it.'),
      '#options' => $codec_levels,
      '#default_value' => !empty($preset->codec_level) ? $preset->codec_level : '4',
    );
    
    
    // advanced audio settings
    $form['settings']['adv_audio'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Advanced audio settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE
    );
    $form['settings']['adv_audio']['audio_bitrate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Audio bitrate'),
      '#description' => $this->t('The overall audio bitrate specified as kilobits per second (kbps, e.g. 96 or 160). This value can\'t exceed 160 kbps per channel. 96-160 is usually a good range for stereo output.'),
      '#default_value' => !empty($preset->audio_bitrate) ? $preset->audio_bitrate : ''
    );
    $form['settings']['adv_audio']['audio_channels'] = array(
      '#type' => 'select',
      '#title' => $this->t('Audio channels'),
      '#description' => $this->t('By default we will choose the lesser of the number of audio channels in the input file or 2 (stereo).'),
      '#options' => array(
        1 => '1 - Mono',
        2 => '2 - Stereo' . ' (' . $this->t('default') . ')'
      ),
      '#default_value' => (!empty($preset->audio_channels)) ? $preset->audio_channels : 2
    );
    $form['settings']['adv_audio']['audio_sample_rate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Audio sample rate'),
      '#description' => $this->t('The sample rate of the audio in hertz. Manually setting this may cause problems, depending on the selected bitrate and number of channels.'),
      '#default_value' => !empty($preset->audio_sample_rate) ? $preset->audio_sample_rate : ''
    );
    $form['settings']['adv_audio']['skip_audio'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Skip audio'),
      '#description' => $this->t('The audio track will be omitted from the output. You must specify a video format and no audio track will be present in the resulting file.'),
      '#default_value' => !empty($preset->skip_audio) ? $preset->skip_audio : ''
    );

    // Watermark
    $form['settings']['watermark'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Video watermark'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#description' => $this->t('At this moment this only works when using the Zencoder transcoder.'),
    );
    $form['settings']['watermark']['video_watermark_enabled'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Enable watermark video'),
      '#default_value' => !empty($preset->video_watermark_enabled) ? $preset->video_watermark_enabled : FALSE,
    );
    $form['settings']['watermark']['file'] = [
      '#type' => 'container',
      '#states' => array(
        'visible' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
        'required' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
      ),
    ];
    $form['settings']['watermark']['file']['video_watermark_fid'] = array(
      '#type' => 'managed_file',
      '#title' => $this->t('Upload watermark image'),
      '#description' => $this->t('Watermark image should be a PNG or JPG image. The file will be uploaded to %destination.', array('%destination' => $destination)),
      '#default_value' => !empty($preset->video_watermark_fid) ? $preset->video_watermark_fid : 0,
      '#upload_location' => $destination,
      '#upload_validators' => array('file_validate_extensions' => array('jpg png'), 'file_validate_is_image' => array()),
    );
    $form['settings']['watermark']['video_watermark_y'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Top position'),
      '#description' => $this->t('Where to place the watermark relative to the top of the video. Use a negative number to place the watermark relative to the bottom of the video.'),
      '#default_value' => isset($preset->video_watermark_y) ? $preset->video_watermark_y : 5,
      '#size' => 10,
      '#maxlength' => 10,
      '#field_suffix' => 'px',
      '#states' => array(
        'visible' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['settings']['watermark']['video_watermark_width'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Width'),
      '#description' => $this->t('The width of the watermark. Use pixels or append a % sign to indicate a percentage relative to the width of the video. If left empty, the width will be the original width maximized by the video width.'),
      '#default_value' => isset($preset->video_watermark_width) ? $preset->video_watermark_width : '',
      '#size' => 10,
      '#maxlength' => 10,
      '#states' => array(
        'visible' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['settings']['watermark']['video_watermark_height'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Height'),
      '#description' => $this->t('The height of the watermark. Use pixels or append a % sign to indicate a percentage relative to the height of the video. If left empty, the width will be the original height maximized by the video height.'),
      '#default_value' => isset($preset->video_watermark_width) ? $preset->video_watermark_width : '',
      '#size' => 10,
      '#maxlength' => 10,
      '#states' => array(
        'visible' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['settings']['watermark']['video_watermark_origin'] = array(
      '#title' => $this->t('Origin'),
      '#type' => 'select',
      '#options' => array(
        'content' => $this->t('content: visible video area') . ' (' . $this->t('default') . ')',
        'frame' => $this->t('frame: video area including padding'),
      ),
      '#default_value' => isset($preset->video_watermark_origin) ? $preset->video_watermark_origin : 'content',
      '#states' => array(
        'visible' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
      ),
    );
    /*
    Not supported by Zencoder anyway
    $form['settings']['watermark']['video_watermark_onlyforaudio'] = array(
      '#type' => 'checkbox',
      '#title' => t('Only add watermark for audio files'),
      '#description' => t('Use this function to create video files using an audio input file and a static image.'),
      '#default_value' => !empty($preset->video_watermark_onlyforaudio) ? $preset->video_watermark_onlyforaudio : FALSE,
      '#states' => array(
        'visible' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
      ),
    );
    */
    // video optimizations
    $form['settings']['vid_optimization'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Video optimization'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE
    );
    $form['settings']['vid_optimization']['autolevels'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Autolevels'),
      '#description' => $this->t('Automatic brightness / contrast correction.'),
      '#default_value' => !empty($preset->autolevels) ? $preset->autolevels : ''
    );
    $form['settings']['vid_optimization']['deblock'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Deblock'),
      '#description' => $this->t('Apply deblocking filter. Useful for highly compressed or blocky input videos.'),
      '#default_value' => !empty($preset->deblock) ? $preset->deblock : ''
    );
    $form['settings']['vid_optimization']['denoise'] = array(
      '#type' => 'select',
      '#title' => $this->t('Denoise'),
      '#description' => $this->t('Apply denoise filter. Generally results in slightly better compression and slightly slower encoding. Beware of any value higher than "Weak" (unless you\'re encoding animation).'),
      '#options' => array(
        '' => $this->t('None'),
        'weak' => 'Weak - usually OK for general use',
        'medium' => 'Medium',
        'strong' => 'Strong - beware',
        'strongest' => 'Strongest - beware, except for Anime'
      ),
      '#default_value' => (!empty($preset->denoise)) ? $preset->denoise : 2
    );
    
    // Create clip
    $form['settings']['create_clip'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Create clip'),
      '#collapsible' => TRUE,
      '#collapsed' => empty($preset->clip_start) && empty($preset->clip_length),
    );
    $form['settings']['create_clip']['clip_start'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Start clip'),
      '#description' => $this->t('The starting point of a subclip (in hh:mm:ss.s or number of seconds).'),
      '#default_value' => !empty($preset->clip_start) ? $preset->clip_start : '',
    );
    $form['settings']['create_clip']['clip_length'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Clip length'),
      '#description' => $this->t('The length of the subclip (in hh:mm:ss.s or number of seconds).'),
      '#default_value' => !empty($preset->clip_length) ? $preset->clip_length : '',
    );
    return $form;
  }

  /**
   * Checks for an existing presets.
   *
   * @param string|int $entity_id
   *   The entity ID.
   * @param array $element
   *   The form element.
   * @param FormStateInterface $form_state
   *   The form state.
   *
   * @return bool
   *   TRUE if this format already exists, FALSE otherwise.
   */
  public function exists($entity_id, array $element, FormStateInterface $form_state) {
    // Use the query factory to build a new preset entity query.
    $query = $this->entityQueryFactory->get('video_transcode_preset');

    // Query the entity ID to see if its in use.
    $result = $query->condition('id', $element['#field_prefix'] . $entity_id)
      ->execute();

    // We don't need to return the ID, only if it exists or not.
    return (bool) $result;
  }

  /**
   * Overrides Drupal\Core\Entity\EntityFormController::actions().
   *
   * To set the submit button text, we need to override actions().
   *
   * @param array $form
   *   An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   An associative array containing the current state of the form.
   *
   * @return array
   *   An array of supported actions for the current entity form.
   */
  protected function actions(array $form, FormStateInterface $form_state) {
    // Get the basic actins from the base class.
    $actions = parent::actions($form, $form_state);

    // Change the submit button text.
    $actions['submit']['#value'] = $this->t('Save');

    // Return the result.
    return $actions;
  }

  /**
   * Overrides Drupal\Core\Entity\EntityFormController::validate().
   *
   * @param array $form
   *   An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   An associative array containing the current state of the form.
   */
  public function validate(array $form, FormStateInterface $form_state) {
    parent::validate($form, $form_state);

    // Add code here to validate your config entity's form elements.
    // Nothing to do here.
  }

  /**
   * Overrides Drupal\Core\Entity\EntityFormController::save().
   *
   * Saves the entity. This is called after submit() has built the entity from
   * the form values. Do not override submit() as save() is the preferred
   * method for entity form controllers.
   *
   * @param array $form
   *   An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   An associative array containing the current state of the form.
   */
  public function save(array $form, FormStateInterface $form_state) {
    // EntityForm provides us with the entity we're working on.
    $preset = $this->getEntity();
    // Drupal already populated the form values in the entity object. Each
    // form field was saved as a public variable in the entity class. PHP
    // allows Drupal to do this even if the method is not defined ahead of
    // time.
    $status = $preset->save();

    // Grab the URL of the new entity. We'll use it in the message.
    $url = $preset->toUrl();

    // Create an edit link.
    $edit_link = Link::fromTextAndUrl($this->t('Edit'), $url)->toString();

    if ($status == SAVED_UPDATED) {
      // If we edited an existing entity...
      drupal_set_message($this->t('Preset %label has been updated.', array('%label' => $preset->label())));
      $this->logger('video_transcode')->notice('Preset %label has been updated.', ['%label' => $preset->label(), 'link' => $edit_link]);
    }
    else {
      // If we created a new entity...
      drupal_set_message($this->t('Preset %label has been added.', array('%label' => $preset->label())));
      $this->logger('video_transcode')->notice('Preset %label has been added.', ['%label' => $preset->label(), 'link' => $edit_link]);
    }

    // Redirect the user back to the listing route after the save operation.
    $form_state->setRedirect('entity.video_transcode_preset.list');
  }

}