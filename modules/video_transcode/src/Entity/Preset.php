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
 *   label = @Translation("Preset"),
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
   * The output video size.
   *
   * @var string
   */
  public $video_wxh;

  /**
   * The output video frame rate.
   *
   * @var string
   */
  public $frame_rate;

  /**
   * The output video bitrate.
   *
   * @var string
   */
  public $video_bitrate;

  /**
   * The output audio bitrate.
   *
   * @var string
   */
  public $audio_bitrate;

  /**
   * The output video codec.
   *
   * @var string
   */
  public $video_codec;

  /**
   * The output audio codec.
   *
   * @var string
   */
  public $audio_codec;

  /**
   * The output video h264 profile.
   *
   * @var string
   */
  public $h264_profile;

  /**
   * The output video reference frames.
   *
   * @var string
   */
  public $video_reference_frames;

  /**
   * The output video codec level.
   *
   * @var string
   */
  public $video_codec_level;

}