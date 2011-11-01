<?php

/**
 * Field handler to provide simple renderer that turns a URL into a clickable link.
 */
class video_handler_field_video_duration extends views_handler_field {

  function render($values) {
    if (!empty($values->video_queue_duration))
      return gmdate("H:i:s", $values->video_queue_duration);
    else
      return NULL;
  }

}
