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
      if (data.format == 'html') {
        // Prevent duplicating
        if ($(data.node).is('img.wysiwyg-video')) {
          return;
        }
        var content = this._getPlaceholder(settings);
      }
      else {
        // Prevent duplicating.
        // @todo data.content is the selection only; needs access to complete content.
        if (data.content.match(/[content:video]/)) {
          return;
        }
        var content = '[content:video]';
      }
      if (typeof content != 'undefined') {
        Drupal.wysiwyg.instances[instanceId].insert(content);
      }
    },

    /**
   * Replace all [[content:video]] tags with images.
   */
    attach: function(content, settings, instanceId) {
      content = content.replace(/\[content:video\]/g, this._getPlaceholder(settings));
      return content;
    },

    /**
   * Replace images with [[content:video]] tags in content upon detaching editor.
   */
    detach: function(content, settings, instanceId) {
      var $content = $('<div>' + content + '</div>');
      $.each($('img.wysiwyg-video', $content), function (i, elem) {
        var tag = ('[content:video]');
        $(this).replaceWith(tag);
      });
      return $content.html();
    },

    /**
   * Helper function to return a HTML placeholder.
   */
    _getPlaceholder: function (settings) {
      return '<img src="' + settings.path + '/images/spacer.gif" alt="Video" title="Video" class="wysiwyg-video drupal-content" height="325px" />';
    }
  };

})(jQuery);
