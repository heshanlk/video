(function ($) {

  Drupal.wysiwyg.plugins['video'] = {

    /**
   * Return whether the passed node belongs to this plugin.
   */
    isNode: function(node) {
      return ($(node).is('img.wysiwyg-video'));
    },

    /**
   * Execute the button.
   */
    invoke: function(data, settings, instanceId) { 
      Drupal.behaviors.video_wysiwyg.videoBrowser(function(nid, data, settings, instanceId){
        if (data.format == 'html') {
          // Prevent duplicating
          if ($(data.node).is('img.wysiwyg-video')) {
            return;
          }
          var content = Drupal.wysiwyg.plugins['video']._getPlaceholder(settings, Drupal.settings.basePath +'video/embed/' +nid);
        }
        else {
          // Prevent duplicating.
          // @todo data.content is the selection only; needs access to complete content.
          if (data.content.match(/[content:video]/)) {
            return;
          }
          var content = nid;
        }
        if (typeof content != 'undefined') {
          Drupal.wysiwyg.instances[instanceId].insert(content);
        }
      }, data, settings, instanceId);
    },

    /**
   * Replace all [[content:video]] tags with images.
   */
    attach: function(content, settings, instanceId) {
      return content;
    },

    /**
   * Replace images with [[content:video]] tags in content upon detaching editor.
   */
    detach: function(content, settings, instanceId) {
      //      return $content.html();
      return content;
    },

    /**
   * Helper function to return a HTML placeholder.
   */
    _getPlaceholder: function (settings, src) {
      return '<iframe width="420" height="315" src="'+src+'" frameborder="0" allowfullscreen></iframe>';
    }
  };

})(jQuery);
