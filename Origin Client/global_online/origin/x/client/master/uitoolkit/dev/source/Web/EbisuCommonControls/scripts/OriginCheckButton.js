var ValidationStatus = { 
	Normal: "Normal", 
	Validating: "Validating", 
	Valid: "Valid", 
	Invalid: "Invalid" 
};

(function(){
	origin.toolkit.CheckButton = function(options){
	    var $this = this;
	
		this.defaults = $.extend({
			enabled: true,
			checked: false,
			text: "", 
			id: "",
			name: "",
			value: "",
			validationStatus : ValidationStatus.Normal
		},options);
	
		this.checkBoxElement = $(document.createElement('input')).attr("type", "checkbox");		
		this.labelElement = $(document.createElement('label'));		
		this.checkBoxElement.addClass('originCheckBox defaultCheckBox');
		this.labelElement.addClass('originCheckBoxLabel');
		
		if (this.defaults.id != "") {
			this.checkBoxElement.attr("id", this.defaults.id);
			this.labelElement.attr('for', this.defaults.id);	
		}
		
		if(this.defaults.name != "") this.checkBoxElement.attr("name", this.defaults.name);
		if (this.defaults.value != "") {
			this.checkBoxElement.attr("value", this.defaults.value);
		}
		
		if (this.defaults.text != "") {
			this.labelElement.text(this.defaults.text)
		}
		
		this.setValidationStatus(this.defaults.validationStatus);
		this.setEnabled(this.defaults.enabled);
		this.setChecked(this.defaults.checked);
				
		this.element = this.checkBoxElement.after(this.labelElement);
	}

	origin.toolkit.CheckButton.prototype = {		
		setText: function(text){
			this.labelElement.text(text);
		},
		setFocus : function(){
			this.checkBoxElement.focus();			
		},		
		setEnabled : function(enabled){			
			if (enabled) {
				this.checkBoxElement.removeAttr("disabled", "disabled");				
				this.checkBoxElement.removeAttr("tabindex", "-1");
				this.labelElement.css('color', '#424242');
				return this.checkBoxElement.removeClass("disabledCheckBox");
			}
			else {
				this.checkBoxElement.attr("disabled", "disabled");
				this.checkBoxElement.attr("tabIndex", "-1");
				this.labelElement.css('color', '#B0B0B0');
				return this.checkBoxElement.addClass("disabledCheckBox");
			}
		},		
		setChecked: function(checked){
			if(checked) this.checkBoxElement.attr("checked", "checked");
			else this.checkBoxElement.removeAttr("checked");
		},
		
		setValidationStatus : function(status){
			this.defaults.validationStatus = status;
			switch(status){
				case ValidationStatus.Invalid:
					if(this.checkBoxElement.hasClass("defaultCheckBox")) this.checkBoxElement.removeClass("defaultCheckBox");
					this.checkBoxElement.addClass("errorCheckBox");
					break;
				default:
					if(this.checkBoxElement.hasClass("errorCheckBox")) this.checkBoxElement.removeClass("errorCheckBox");
					this.checkBoxElement.addClass("defaultCheckBox");
					break;
			}
		},		
		getValidationStatus: function(){
			return this.defaults.validationStatus;
		}	
	}
}).call(this);
