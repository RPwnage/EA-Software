//Namespace.Register allows you to register namespaces globally in your page
Namespace.Register("origin.toolkit");

/*
 * @param {Object} options
 */

var ValidationStatus = { 
	Normal: "Normal", 
	Validating: "Validating", 
	Valid: "Valid", 
	Invalid: "Invalid" 
};

(function(){
	origin.toolkit.SingleLineEdit = function(options){
		var $this = this;	
		this.defaults = $.extend({
			maxLength: 64,
			readOnly: false,
			enabled: true,
			id: "",
			validationStatus: ValidationStatus.Normal,
			password: false,
			placeHolderText: "",
			inputAccessory: ""
		},options);		
		
		this.element = $(document.createElement('input'));
		if(this.defaults.id != "") this.element.attr("id", this.defaults.id);
		this.element.addClass("textbox defaultTextBox");		
		this.element.attr("maxLength", this.defaults.maxLength);		
		if(this.defaults.readOnly) this.element.attr("readonly", "readonly");
		
		if(this.defaults.placeHolderText != "" && this.defaults.inputAccessory == ""){
			this.element.css("color", "#888");
			this.element.attr("value", this.defaults.placeHolderText);
			
			this.element.focus(function(){ 
				if($this.element.val() == $this.defaults.placeHolderText){
					$this.element.attr("value", "");
					$this.element.css("color", "#3e3d3f");
				}
			});
			this.element.blur(function(){
				if($this.element.val() == ""){
					$this.element.attr("value", $this.defaults.placeHolderText);
					$this.element.css("color", "#888");
				}
			});
		}
	
		if(this.defaults.password) this.element.attr("type", "password");		
		if(!this.defaults.enabled) this.element.attr("disabled", "disabled");		
		if(this.defaults.error){
			if(this.element.hasClass("defaultTextBox")) this.element.removeClass("defaultTextBox");
			this.element.addClass("errorTextBox");
		}		
		this.clickEvent = $.Callbacks();		
        this.inputAccessory = $.Callbacks();

		this.element.bind('click', function(e){
			return $this.clickEvent.fire();
		});	
	
		
		if(this.defaults.inputAccessory == "Search"){
			this.searchElem = $(document.createElement('span'));		
			this.parent = $(document.createElement('div')).attr('class', 'searchIcon');
			
			this.element.css('padding-right', '20px');
			this.element = this.parent.append(this.element,this.searchElem);
			var input = this.element.find('input');
			input.css("color", "#888");
			input.attr("value", this.defaults.placeHolderText);
			
			input.focus(function(){ 
				if(input.val() == $this.defaults.placeHolderText){
					input.attr("value", "");
					input.css("color", "#3e3d3f");
				}
			});
			input.blur(function(){ 
				if(input.val() == ""){
					input.attr("value", $this.defaults.placeHolderText);
					input.css("color", "#888");
				}
			});						
			
			this.searchElem.bind('click', function(e){
				return $this.inputAccessory.fire();
			});	
		}
	}
	
	origin.toolkit.SingleLineEdit.prototype = {
		setText : function(text){
			if (this.defaults.inputAccessory != "") { console.info('i am in');
				this.element.find('input').css("color", "#3e3d3f");
				return this.element.find('input').attr("value", text);
			}
			else {
				this.element.css("color", "#3e3d3f");
				return this.element.attr("value", text);
			}
		},
		getText : function(){
			if(this.defaults.inputAccessory != "")
				return this.element.find('input').val(); 
			else 
				return this.element.val();
		},
		getPlaceHolderText : function(){
			return this.defaults.placeHolderText;
		},
		setValidationStatus : function(status){
			if (this.defaults.inputAccessory == "Search") { // for now just putting a check for search add later for caps lock and other
				return;
			}
			switch(status) {
				case ValidationStatus.Valid:
					if(this.element.hasClass("defaultTextBox")) this.element.removeClass("defaultTextBox");
					if(this.element.hasClass("errorTextBox")) this.element.removeClass("errorTextBox");
					this.element.addClass("validTextBox");
					break;
				case ValidationStatus.Invalid:
					if(this.element.hasClass("defaultTextBox")) this.element.removeClass("defaultTextBox");
					if(this.element.hasClass("validTextBox")) this.element.removeClass("validTextBox");
					this.element.addClass("errorTextBox");
					break;
				default:
					if(this.element.hasClass("errorTextBox")) this.element.removeClass("errorTextBox");
					this.element.addClass("defaultTextBox");
					break;
			}
		},
		setEnabled : function(bool) {
			if (this.defaults.inputAccessory != "") {
				if (bool) this.element.find('input').removeAttr("disabled", "disabled");
				else this.element.find('input').attr("disabled", "disabled");
			}
			else {
				if (bool) this.element.removeAttr("disabled", "disabled");
				else this.element.attr("disabled", "disabled");
			}
		},
		setReadOnly : function(bool) {
			if (this.defaults.inputAccessory != "") {
				if (bool) this.element.find('input').attr("readonly", "readonly");
				else this.element.find('input').removeAttr("readonly");
			}else{
				if (bool) this.element.attr("readonly", "readonly");
				else this.element.removeAttr("readonly");	
			}
		}
	}
}).call(this);