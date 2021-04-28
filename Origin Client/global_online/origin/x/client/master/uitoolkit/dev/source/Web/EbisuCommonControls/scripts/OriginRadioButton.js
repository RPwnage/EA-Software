/**
 * @author jledbetter
 */

var ValidationStatus = { 
	Normal: "Normal", 
	Validating: "Validating", 
	Valid: "Valid", 
	Invalid: "Invalid" 
};

(function(){
	origin.toolkit.RadioButton = function(options){
	    var $this = this;
	
		this.defaults = $.extend({
			enabled: true,
			checked: false,
			text: "",
			id: "",
			name: "",
			value: "",
			validationStatus: ValidationStatus.Normal,
		},options);
	
		this.radioButtonElement = $(document.createElement("input")).attr("type", "radio");	
		this.labelElement = $(document.createElement("label"));
		this.radioButtonElement.addClass("originRadioButton defaultRadioButton");
		this.labelElement.addClass("originRadioButtonLabel");	
	
		if(this.defaults.name != "") this.radioButtonElement.attr("name", this.defaults.name);
		if(this.defaults.id != "") {
			this.radioButtonElement.attr("id", this.defaults.id);
			this.labelElement.attr("for", this.defaults.id);
		}
		if(this.defaults.value != "") this.radioButtonElement.attr("value", this.defaults.value);
		
		if (this.defaults.text != "") this.labelElement.text(this.defaults.text)
	
		this.setValidationStatus(this.defaults.validationStatus);
		this.setEnabled(this.defaults.enabled);
		this.setChecked(this.defaults.checked);

				
		this.element = this.radioButtonElement.after(this.labelElement);
	}

	origin.toolkit.RadioButton.prototype = {
		setText : function(text) {
			this.labelElement.text(text);
		},

		setFocus : function() {
			this.radioButtonElement.focus();	
		},
		
		setChecked : function(checked) {
			var $this = this;
			if (checked) $this.radioButtonElement.attr("checked", "checked");
			else $this.radioButtonElement.removeAttr("checked");
		},
		
		setEnabled : function(enabled) {
			if (enabled) {
				this.radioButtonElement.removeAttr("disabled", "disabled");
				this.radioButtonElement.removeAttr("tabindex", "-1");
				this.labelElement.removeClass("disabled");
				return this.radioButtonElement.removeClass("disabled");
			}
			else {
				this.radioButtonElement.attr("disabled", "disabled");
				this.radioButtonElement.attr("tabIndex", "-1");
				this.labelElement.addClass("disabled");
				return this.radioButtonElement.addClass("disabled");
			}
		},
		
		setValidationStatus : function(status){
			switch(status) {
				case ValidationStatus.Invalid:
					if(this.radioButtonElement.hasClass("defaultRadioButton")) this.radioButtonElement.removeClass("defaultRadioButton");
					this.radioButtonElement.addClass("errorRadioButton");
					break;
				default:
					if(this.radioButtonElement.hasClass("errorRadioButton")) this.radioButtonElement.removeClass("errorRadioButton");
					this.radioButtonElement.addClass("defaultRadioButton");
					break;
			}
		},
		
		getValidationStatus : function() {
			return this.defaults.validationStatus;
		}
	}
}).call(this);

