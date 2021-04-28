(function(){
	origin.toolkit.InfoTip = function(options){
		this.defaults = $.extend({
			text: "",
			tip: "",
			size: "",
		},options);
		
		this.a = $(document.createElement('a'));
		this.a.attr("href", "#");
		this.a.attr("class", "originToolTip");
		this.span = $(document.createElement('span'));
		this.a.text(this.defaults.text);
					console.log(this.defaults.size);
		if (this.defaults.size == "medium") {
			this.span.attr("class", "medium");
		}
		else if (this.defaults.size == "large") {
			this.span.attr("class", "large");
		}
		
		this.span.text(this.defaults.tip);
		this.element = this.a.append(this.span);
	}
	
	origin.toolkit.InfoTip.prototype = {

		setTip : function(text, tip) {
			var mtext = document.createTextNode(text);
			var mtip = document.createElement("span");
			mtip.appendChild(document.createTextNode(tip));
			this.element.html(text + mtip.innerHTML);
		},		
	}
}).call(this);

