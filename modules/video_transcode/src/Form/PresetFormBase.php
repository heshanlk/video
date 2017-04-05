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
    );
    // video settings
    $form['settings']['video'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Video settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['settings']['video']['video_extension'] = array(
      '#type' => 'select',
      '#title' => $this->t('Video output extension'),
      '#description' => $this->t('Extension of the output video.'),
      '#options' => [],
      '#default_value' => !empty($settings['video_extension']) ? $settings['video_extension'] : NULL,
      '#required' => TRUE,
    );

    $form['settings']['video']['video_codec'] = array(
      '#type' => 'select',
      '#title' => $this->t('Video codec'),
      '#description' => $this->t('The video codec used in the video file can affect the ability to play the video on certain devices.'),
      '#options' => [],
      '#required' => $defaultvideocodec === NULL,
      '#default_value' => !empty($settings['video_codec']) ? $settings['video_codec'] : $defaultvideocodec,
    );

    $form['settings']['video']['video_preset'] = array(
      '#type' => 'select',
      '#title' => $this->t('FFmpeg video preset'),
      '#description' => $this->t('A preset file contains a sequence of option=value pairs, one for each line, specifying a sequence of options which would be awkward to specify on the command line. Lines starting with the hash (\'#\') character are ignored and are used to provide comments. Check the &quot;presets&quot; directory in the FFmpeg source tree for examples. See the !doc. Newer FFmpeg installations do not supply libx264 presets anymore, so &quot;!optionnamenone&quot; should be selected. If FFmpeg fails with an error related to presets, please also select &quot;!optionnamenone&quot;. In other cases, an error message may suggest that you should select one of the available options. This setting requires some experimentation.', array('!doc' => \Drupal::l($this->t('FFmpeg documentation'), Url::fromUri('http://ffmpeg.org/ffmpeg.html#Preset-files')), '!optionnamenone' => $this->t('None'))),
      '#options' => array(
        '' => $this->t('None'),
        'libx264-baseline' => 'libx264-baseline',
        'libx264-default' => 'libx264-default',
        'libx264-fast' => 'libx264-fast',
        'libx264-faster' => 'libx264-faster',
        'libx264-hq' => 'libx264-hq',
        'libx264-ipod320' => 'libx264-ipod320',
        'libx264-ipod640' => 'libx264-ipod640',
        'libx264-main' => 'libx264-main',
        'libx264-max' => 'libx264-max',
        'libx264-medium' => 'libx264-medium',
        'libx264-normal' => 'libx264-normal',
        'libx264-slow' => 'libx264-slow',
        'libx264-slower' => 'libx264-slower',
        'libx264-superfast' => 'libx264-superfast',
        'libx264-ultrafast' => 'libx264-ultrafast',
        'libx264-veryfast' => 'libx264-veryfast',
        'libvpx-1080p' => 'libvpx-1080p',
        'libvpx-1080p50_60' => 'libvpx-1080p50_60',
        'libvpx-360p' => 'libvpx-360p',
        'libvpx-720p' => 'libvpx-720p',
        'libvpx-720p50_60' => 'libvpx-720p50_60',
        'libx264-lossless_fast' => 'libx264-lossless_fast',
        'libx264-lossless_max' => 'libx264-lossless_max',
        'libx264-lossless_medium' => 'libx264-lossless_medium',
        'libx264-lossless_slow' => 'libx264-lossless_slow',
        'libx264-lossless_slower' => 'libx264-lossless_slower',
        'libx264-lossless_ultrafast' => 'libx264-lossless_ultrafast',
      ),
      '#default_value' => (!empty($settings['video_preset'])) ? $settings['video_preset'] : '',
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
      '#default_value' => (!empty($settings['video_quality'])) ? $settings['video_quality'] : 3,
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
      '#default_value' => (!empty($settings['video_speed'])) ? $settings['video_speed'] : 3,
    );
    $form['settings']['video']['wxh'] = array(
      '#type' => 'select',
      '#title' => $this->t('Dimensions'),
      '#description' => $this->t('Select the desired widthxheight of the video player. You can add your own dimensions from !settings.', array('!settings' => \Drupal::l($this->t('Video module settings'), Url::fromUri('internal:/admin/config/media/video')))),
      '#default_value' => !empty($settings['wxh']) ? $settings['wxh'] : '640x360',
      '#options' => [],
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
      '#default_value' => (!empty($settings['video_aspectmode'])) ? $settings['video_aspectmode'] : 'preserve',
    );
    $form['settings']['video']['video_upscale'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Upscale'),
      '#description' => $this->t('If the input file is smaller than the target output, should the file be upscaled to the target size?'),
      '#default_value' => !empty($settings['video_upscale']) ? $settings['video_upscale'] : ''
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
      '#options' => [],
      '#required' => $defaultaudiocodec === NULL,
      '#default_value' => (!empty($settings['audio_codec'])) ? $settings['audio_codec'] : $defaultaudiocodec,
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
      '#default_value' => (!empty($settings['audio_quality'])) ? $settings['audio_quality'] : 3,
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
      '#default_value' => (!empty($settings['deinterlace'])) ? $settings['deinterlace'] : 'detect'
    );
    $form['settings']['adv_video']['max_frame_rate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Maximum frame rate'),
      '#description' => $this->t('A maximum frame rate cap (in frames per second).'),
      '#default_value' => !empty($settings['max_frame_rate']) ? $settings['max_frame_rate'] : ''
    );
    $form['settings']['adv_video']['frame_rate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Frame rate'),
      '#description' => $this->t('Force a specific output frame rate (in frames per second). For best quality, do not use this setting.'),
      '#default_value' => !empty($settings['frame_rate']) ? $settings['frame_rate'] : ''
    );
    $form['settings']['adv_video']['keyframe_interval'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Key frame interval'),
      '#description' => $this->t('By default, a keyframe will be created at most every 250 frames. Specifying a different keyframe interval will allow you to create more or fewer keyframes in your video. A greater number of keyframes will increase the size of your output file, but will allow for more precise scrubbing in most players. Keyframe interval should be specified as a positive integer. For example, a value of 100 will create a keyframe every 100 frames.'),
      '#default_value' => !empty($settings['keyframe_interval']) ? $settings['keyframe_interval'] : ''
    );
    $form['settings']['adv_video']['video_bitrate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Video bitrate'),
      '#description' => $this->t('A target bitrate in kbps. Not necessary if you select a Video Quality setting, unless you want to target a specific bitrate.'),
      '#default_value' => !empty($settings['video_bitrate']) ? $settings['video_bitrate'] : '',
    );
    $form['settings']['adv_video']['bitrate_cap'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Bitrate cap'),
      '#description' => $this->t('A bitrate cap in kbps, used for streaming servers.'),
      '#default_value' => !empty($settings['bitrate_cap']) ? $settings['bitrate_cap'] : ''
    );
    $form['settings']['adv_video']['buffer_size'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Buffer size'),
      '#description' => $this->t('The buffer size for the bitrate cap in kbps.'),
      '#default_value' => !empty($settings['buffer_size']) ? $settings['buffer_size'] : ''
    );
    $form['settings']['adv_video']['one_pass'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Force one-pass encoding'),
      '#default_value' => !empty($settings['one_pass']) ? $settings['one_pass'] : ''
    );
    $form['settings']['adv_video']['skip_video'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Skip video'),
      '#description' => $this->t('The video track will be omitted from the output. You can still specify a video format, however, no video track will be present in the resulting file.'),
      '#default_value' => !empty($settings['skip_video']) ? $settings['skip_video'] : ''
    );
    
    // Not all transcoders support setting the pixel format
    $form['settings']['adv_video']['pixel_format'] = array(
      '#type' => 'select',
      '#title' => $this->t('Pixel format'),
      '#description' => $this->t('The pixel format of the output file. Yuv420p is a safe choice, yuvj420p is not supported by at least Google Chrome. If you select <em>!optionname</em> and the input video is yuvj420p, the output video will not be playable on Chrome.', array('!optionname' => $this->t('Same as input video'))),
      '#options' => [],
      '#default_value' => !empty($settings['pixel_format']) ? $settings['pixel_format'] : '',
    );
    
    $profiles = array('' => $this->t('None'), 'baseline' => 'Baseline', 'main' => 'Main', 'high' => 'High');
    $form['settings']['adv_video']['h264_profile'] = array(
      '#type' => 'select',
      '#title' => $this->t('H.264 profile'),
      '#description' => $this->t('Use Baseline for maximum compatibility with players. Select !optionnamenone when this is not an H.264 preset or when setting the profile causes errors.', array('!optionnamenone' => $this->t('None'))),
      '#options' => $profiles,
      '#default_value' => !empty($settings['h264_profile']) ? $settings['h264_profile'] : '',
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
      '#default_value' => !empty($settings['audio_bitrate']) ? $settings['audio_bitrate'] : ''
    );
    $form['settings']['adv_audio']['audio_channels'] = array(
      '#type' => 'select',
      '#title' => $this->t('Audio channels'),
      '#description' => $this->t('By default we will choose the lesser of the number of audio channels in the input file or 2 (stereo).'),
      '#options' => array(
        1 => '1 - Mono',
        2 => '2 - Stereo' . ' (' . $this->t('default') . ')'
      ),
      '#default_value' => (!empty($settings['audio_channels'])) ? $settings['audio_channels'] : 2
    );
    $form['settings']['adv_audio']['audio_sample_rate'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Audio sample rate'),
      '#description' => $this->t('The sample rate of the audio in hertz. Manually setting this may cause problems, depending on the selected bitrate and number of channels.'),
      '#default_value' => !empty($settings['audio_sample_rate']) ? $settings['audio_sample_rate'] : ''
    );
    $form['settings']['adv_audio']['skip_audio'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Skip audio'),
      '#description' => $this->t('The audio track will be omitted from the output. You must specify a video format and no audio track will be present in the resulting file.'),
      '#default_value' => !empty($settings['skip_audio']) ? $settings['skip_audio'] : ''
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
      '#default_value' => !empty($settings['video_watermark_enabled']) ? $settings['video_watermark_enabled'] : FALSE,
    );
    $form['settings']['watermark']['video_watermark_fid'] = array(
      '#type' => 'managed_file',
      '#title' => $this->t('Upload watermark image'),
      '#description' => $this->t('Watermark image should be a PNG or JPG image. The file will be uploaded to %destination.', array('%destination' => $destination)),
      '#default_value' => !empty($settings['video_watermark_fid']) ? $settings['video_watermark_fid'] : 0,
      '#upload_location' => $destination,
      '#upload_validators' => array('file_validate_extensions' => array('jpg png'), 'file_validate_is_image' => array()),
      '#states' => array(
        'visible' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
        'required' => array(
          ':input[id=edit-video-watermark-enabled]' => array('checked' => TRUE),
        ),
      ),
    );
    $form['settings']['watermark']['video_watermark_y'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Top position'),
      '#description' => $this->t('Where to place the watermark relative to the top of the video. Use a negative number to place the watermark relative to the bottom of the video.'),
      '#default_value' => isset($settings['video_watermark_y']) ? $settings['video_watermark_y'] : 5,
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
      '#default_value' => isset($settings['video_watermark_width']) ? $settings['video_watermark_width'] : '',
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
      '#default_value' => isset($settings['video_watermark_width']) ? $settings['video_watermark_width'] : '',
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
      '#default_value' => isset($settings['video_watermark_origin']) ? $settings['video_watermark_origin'] : 'content',
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
      '#default_value' => !empty($settings['video_watermark_onlyforaudio']) ? $settings['video_watermark_onlyforaudio'] : FALSE,
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
      '#default_value' => !empty($settings['autolevels']) ? $settings['autolevels'] : ''
    );
    $form['settings']['vid_optimization']['deblock'] = array(
      '#type' => 'checkbox',
      '#title' => $this->t('Deblock'),
      '#description' => $this->t('Apply deblocking filter. Useful for highly compressed or blocky input videos.'),
      '#default_value' => !empty($settings['deblock']) ? $settings['deblock'] : ''
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
      '#default_value' => (!empty($settings['denoise'])) ? $settings['denoise'] : 2
    );
    
    // Create clip
    $form['settings']['create_clip'] = array(
      '#type' => 'fieldset',
      '#title' => $this->t('Create clip'),
      '#collapsible' => TRUE,
      '#collapsed' => empty($settings['clip_start']) && empty($settings['clip_length']),
    );
    $form['settings']['create_clip']['clip_start'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Start clip'),
      '#description' => $this->t('The starting point of a subclip (in hh:mm:ss.s or number of seconds).'),
      '#default_value' => !empty($settings['clip_start']) ? $settings['clip_start'] : '',
    );
    $form['settings']['create_clip']['clip_length'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Clip length'),
      '#description' => $this->t('The length of the subclip (in hh:mm:ss.s or number of seconds).'),
      '#default_value' => !empty($settings['clip_length']) ? $settings['clip_length'] : '',
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
    $url = $preset->urlInfo();

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