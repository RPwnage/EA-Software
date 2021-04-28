/**
 * @author psandhu
 */
(function(){
	origin.toolkit.Banner = function(options){	
		this.constants = {
			BannerType: {
				Banner: "Banner",
				Message: "Message"
			},			
			IconType: {
				Error : "Error",
				Success : "Success",
				Info : "Info",
				Notice : "Notice"
			}
		};
		this.defaults = $.extend({
			text: "",
			bannerType: "",
			iconType: ""
		},options);				
		this.element = $(document.createElement('div'));
		this.element.text(this.defaults.text);
		this.setBannerType(this.defaults.bannerType);		
		this.setIconType(this.defaults.iconType);
	};
	
	origin.toolkit.Banner.prototype = {
		setText: function(text){
			this.element.text(text);
		},
		getText: function(){
			return this.element.html();
		},
		setIconType : function(iconType){
			switch(iconType){ 
				case this.constants.IconType.Error:
					if(this.element.is(".success, .info, .notice")){
						this.element.removeClass("success info notice");
					} 
					if(this.defaults.bannerType == "Banner"){
						this.element.css("background-color", "#C93507");
					}  
					this.element.addClass("error");
					break;
				case this.constants.IconType.Success:
					if (this.element.is(".error, .info, .notice")) {
						this.element.removeClass("error info notice");
					}
					if (this.defaults.bannerType == "Banner") {
						this.element.css("background-color", "#5CAB26");
					} 
					this.element.addClass("success");
					break;
				case this.constants.IconType.Info: 
					if (this.element.is(".success, .error, .notice")) {
						this.element.removeClass("success error notice");
					}
					if (this.defaults.bannerType == "Banner") {
						this.element.css("background-color", "#269DCF");
					} 
					this.element.addClass("info");
					break;
				case this.constants.IconType.Notice:
					if (this.element.is(".success, .info, .error")) {
						this.element.removeClass("success info error");
					}
					this.element.addClass("notice");
					break;
				default:
					if (this.element.is(".error, .info, .notice")) this.element.removeClass("error info notice");
					this.element.css("background-color", "#5CAB26"); 
					this.element.addClass("success");
			}
		},
		setBannerType : function(bannerType){ 
			switch(bannerType){ 
				case this.constants.BannerType.Banner: 
					this.element.addClass("banner");
					break;
				case this.constants.BannerType.Message:
					this.element.addClass("message");
					break;
				default:
					this.element.addClass("banner");
			}
		},
		getBannerType : function(){
			return this.defaults.bannerType;
		},
		getIconType : function(){
			return this.defaults.iconType;
		}
	};
}).call(this);