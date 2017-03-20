<?php

namespace Drupal\video_transcode;

use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\user\EntityOwnerInterface;
use Drupal\Core\Entity\EntityChangedInterface;

/**
 * Provides an interface defining a Transcode Job entity.
 */
interface TranscodeJobInterface extends ContentEntityInterface, EntityOwnerInterface, EntityChangedInterface {}