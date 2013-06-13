package {

	import flash.display.Sprite;
	import flash.events.AsyncErrorEvent;
	import flash.events.Event;
	import flash.media.Video;
	import flash.net.NetConnection;
	import flash.net.NetStream;
	import flash.media.SoundTransform;
	import flash.net.URLRequest;
	import flash.net.URLStream;
	import flash.utils.ByteArray;
	import flash.net.NetStreamAppendBytesAction;
	
	[SWF(width=1920, height=1080, backgroundColor = 0, backgroundAlpha="0" )]
	public class Main extends Sprite
	{
		private var video:Video;
		private var netStream:NetStream;
		private var urlStream:URLStream;
		
		public function Main():void 
		{
			var customClient:Object = new Object();
			customClient.onMetaData = metaDataHandler;

			var netConnection:NetConnection = new NetConnection();
			netConnection.connect(null);
			netStream = new NetStream(netConnection);
			
			netStream.client = customClient;
			video = new Video(stage.stageWidth, stage.stageHeight);
			video.name = "Video";

			addChild(video);
			
			video.attachNetStream(netStream);
			
			var url:String = "movie.flv";
			if (stage.loaderInfo.parameters.hasOwnProperty("url")) {
				url = stage.loaderInfo.parameters["url"];
			}
			
			var isMp4:Boolean = stage.loaderInfo.parameters.hasOwnProperty("mp4");
			
			var soundTransform:SoundTransform = new SoundTransform();
			soundTransform.volume = 1;
			netStream.soundTransform = soundTransform;
			
			if (!isMp4) {				
				var urlRequest:URLRequest = new URLRequest(url);
				urlStream = new URLStream();
				urlStream.addEventListener(Event.COMPLETE, completeHandler);
				urlStream.load(urlRequest);
				netStream.play(null);
			} else {
				netStream.play(url);
			}
		} 

		private function metaDataHandler(infoObject:Object):void {
			for (var prop:Object in infoObject){
				trace(prop + " : " + infoObject[prop]);
			}
		}
		
		private function completeHandler(event:Event):void {
			trace("completeHandler: " + event);

			var bytes:ByteArray = new ByteArray();

			urlStream.readBytes(bytes);
			netStream.appendBytes(bytes);
		}
	}
}