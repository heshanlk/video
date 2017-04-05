<?php

namespace Drupal\video_transcode\Entity;

use Drupal\Core\Config\Entity\ConfigEntityBase;

/**
 * Defines the preset entity.
 *
 * The lines below, starting with '@ConfigEntityType,' are a plugin annotation.
 * These define the entity type to the entity type manager.
 *
 * The properties in the annotation are as follows:
 *  - id: The machine name of the entity type.
 *  - label: The human-readable label of the entity type. We pass this through
 *    the "@Translation" wrapper so that the multilingual system may
 *    translate it in the user interface.
 *  - handlers: An array of entity handler classes, keyed by handler type.
 *    - access: The class that is used for access checks.
 *    - list_builder: The class that provides listings of the entity.
 *    - form: An array of entity form classes keyed by their operation.
 *  - entity_keys: Specifies the class properties in which unique keys are
 *    stored for this entity type. Unique keys are properties which you know
 *    will be unique, and which the entity manager can use as unique in database
 *    queries.
 *  - links: entity URL definitions. These are mostly used for Field UI.
 *    Arbitrary keys can set here. For example, User sets cancel-form, while
 *    Node uses delete-form.
 *
 * @see http://previousnext.com.au/blog/understanding-drupal-8s-config-entities
 * @see annotation
 * @see Drupal\Core\Annotation\Translation
 *
 * @ingroup video_transcode
 *
 * @ConfigEntityType(
 *   id = "video_transcode_preset",
 *   label = @Translation("Video Preset"),
 *   admin_permission = "administer video transcode presets",
 *   handlers = {
 *     "access" = "Drupal\video_transcode\PresetAccessController",
 *     "list_builder" = "Drupal\video_transcode\Controller\PresetListBuilder",
 *     "form" = {
 *       "add" = "Drupal\video_transcode\Form\PresetAddForm",
 *       "edit" = "Drupal\video_transcode\Form\PresetEditForm",
 *       "delete" = "Drupal\video_transcode\Form\PresetDeleteForm"
 *     }
 *   },
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "label"
 *   },
 *   links = {
 *     "edit-form" = "/admin/config/media/transcode-preset/manage/{video_transcode_preset}",
 *     "delete-form" = "/admin/config/media/transcode-preset/manage/{video_transcode_preset}/delete"
 *   }
 * )
 */
class Preset extends ConfigEntityBase {

  /**
   * The preset ID.
   *
   * @var string
   */
  public $id;

  /**
   * The preset UUID.
   *
   * @var string
   */
  public $uuid;

  /**
   * The preset label.
   *
   * @var string
   */
  public $label;

  /**
   * The output video extension.
   *
   * @var string
   */
  public $video_extension;


  /**
   * The output video codec.
   *
   * @var string
   */
  public $video_codec;

  /**
   * The output video quality.
   *
   * @var string
   */
  public $video_quality;

  /**
   * The output video speed.
   *
   * @var string
   */
  public $video_speed;

  /**
   * The output video size.
   *
   * @var string
   */
  public $wxh;

  /**
   * The output video aspectmode.
   *
   * @var string
   */
  public $video_aspectmode;

  /**
   * The output video upscale.
   *
   * @var string
   */
  public $video_upscale;

  /**
   * The output audio codec.
   *
   * @var string
   */
  public $audio_codec;

  /**
   * The output audio quality.
   *
   * @var string
   */
  public $audio_quality;

  /**
   * The output video codec level.
   *
   * @var string
   */
  public $deinterlace;

  /**
   * The output video max frame rate.
   *
   * @var string
   */
  public $max_frame_rate;

  /**
   * The output video frame rate.
   *
   * @var string
   */
  public $frame_rate;

  /**
   * The output video keyframe interval.
   *
   * @var string
   */
  public $keyframe_interval;

  /**
   * The output video bitrate.
   *
   * @var string
   */
  public $video_bitrate;

  /**
   * The output video bitrate cap.
   *
   * @var string
   */
  public $bitrate_cap;

  /**
   * The output video buffer size.
   *
   * @var string
   */
  public $buffer_size;

  /**
   * The output video one pass.
   *
   * @var string
   */
  public $one_pass;

  /**
   * The output video skip video.
   *
   * @var string
   */
  public $skip_video;

  /**
   * The output video reference frames.
   *
   * @var string
   */
  public $reference_frames;

  /**
   * The output video h264 profile.
   *
   * @var string
   */
  public $h264_profile;

  /**
   * The output video codec level.
   *
   * @var string
   */
  public $codec_level;

  /**
   * The output video audio bitrate.
   *
   * @var string
   */
  public $audio_bitrate;

  /**
   * The output video audio_channels.
   *
   * @var string
   */
  public $audio_channels;

  /**
   * The output video audio_sample_rate.
   *
   * @var string
   */
  public $audio_sample_rate;

  /**
   * The output video skip_audio.
   *
   * @var string
   */
  public $skip_audio;

  /**
   * The output video_watermark_enabled.
   *
   * @var string
   */
  public $video_watermark_enabled;

  /**
   * The output video_watermark_fid.
   *
   * @var string
   */
  public $video_watermark_fid;

  /**
   * The output video_watermark_y.
   *
   * @var string
   */
  public $video_watermark_y;

  /**
   * The output video_watermark_width.
   *
   * @var string
   */
  public $video_watermark_width;

  /**
   * The output video_watermark_height.
   *
   * @var string
   */
  public $video_watermark_height;

  /**
   * The output video_watermark_origin.
   *
   * @var string
   */
  public $video_watermark_origin;

  /**
   * The output video codec level.
   *
   * @var string
   */
  public $autolevels;

  /**
   * The output video codec level.
   *
   * @var string
   */
  public $deblock;

  /**
   * The output video codec level.
   *
   * @var string
   */
  public $denoise;

  /**
   * The output clip_start.
   *
   * @var string
   */
  public $clip_start;

  /**
   * The output clip_length.
   *
   * @var string
   */
  public $clip_length;


}