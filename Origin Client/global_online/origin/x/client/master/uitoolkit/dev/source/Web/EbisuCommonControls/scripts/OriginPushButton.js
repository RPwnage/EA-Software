/**
 * @author jledbetter
 */

(function(){
	origin.toolkit.PushButton = function(options){
	    var $this = this;
	
		this.defaults = $.extend({
			enabled: false,
			text: "Submit",
			href: "javascript:void(0);",
			defaultButton: false,
			buttonType: "Tertiary",
		},options);
	
		this.element = $(document.createElement('a'));
		this.element.attr("href", this.defaults.href);
	
		this.element.addClass("originPushButton");
	
		this.setButtonType(this.defaults.buttonType);
	
		if(!this.defaults.enabled) {
			this.element.attr("disabled", "disabled");
			this.element.attr("tabIndex", "-1");
			this.element.addClass("disabled");
		}
	
		if(options.id != "") this.element.attr("id", options.id);

		this.setText(this.defaults.text);
		this.clickEvent = $.Callbacks();
	
		if (this.defaults.defaultButton) {
			this.setDefaultButton();
		}

		this.element.bind('mousedown', function(event) {
			if (event.which !== 1) return;
			if (!$this.element.hasClass("disabled")) {
	            return $this.element.addClass("pressed");
	        }
		});
	
	    this.element.bind('mouseup', function (event) {
	        if (event.which !== 1) return;
	        if (!$this.element.hasClass("disabled")) {
	            $this.element.removeClass("pressed");
	            return $this.clickEvent.fire();
	        }
	    });
	
	    this.element.bind('mouseout', function (event) {
	        $this.element.removeClass("pressed");
	    });
	}
	
	origin.toolkit.PushButton.prototype = {
		setEnabled : function(enabled){
			if (enabled) {
				this.element.removeAttr("disabled", "disabled");
				this.element.removeAttr("tabindex", "-1");
				return this.element.removeClass("disabled");
			}
			else {
				this.element.attr("disabled", "disabled");
				this.element.attr("tabIndex", "-1");
				return this.element.addClass("disabled");
			}
		},
		isEnabled : function() {
			return !this.element.hasClass("disabled");
		},
		
		setText : function(text) {
			var span = document.createElement("span");
			var bolding = document.createElement("b");
			span.appendChild(bolding);
			bolding.appendChild(document.createTextNode(text));
			this.element.html(span.innerHTML);
		},
		
		setButtonType : function(buttonType) {
			if (buttonType != "") {
				this.element.removeClass("buttonPrimary buttonSecondary buttonTertiary");
	
				if (buttonType == "Primary") this.element.addClass("buttonPrimary");
				else if (buttonType == "Secondary") this.element.addClass("buttonSecondary");
				else this.element.addClass("buttonTertiary")	
			}
		},
	
		setDefaultButton : function() {
			var $this = this;
			$(document).keypress(function(e) {
			  if(e.which == 13){
			  	$this.clickEvent.fire();
			  }
			});	
		},
		
		setFocus : function() {
			this.element.focus();
		}
	}
}).call(this);
/*
 * Whether or not to show the antrails
 * if something had been tabbed to, start showing antrails
 * else no ant trails
*/
var tabbed = false;
$(document).keydown(function(event){
	if ((event.keyCode == 9) && (!tabbed)) {
 		var sheet = document.createElement('style')
		sheet.innerHTML = ".originPushButton:focus, .originPushButton.pressed, .originPushButton:focus:hover {outline: 1px dotted #000;outline-offset: -2px;}";
		sheet.innerHTML += ".buttonTertiary:focus, .buttonTertiary.pressed, .buttonTertiary:focus:hover {outline-offset: -3px;}";
		document.body.appendChild(sheet);
		tabbed = true;	
 	}
});
