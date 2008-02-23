//
//  PlaylistEntry.m
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistEntry.h"
#import "CogAudio/AudioPropertiesReader.h"
#import "CogAudio/AudioMetadataReader.h"

@implementation PlaylistEntry

@synthesize index;
@synthesize shuffleIndex;
@synthesize status;
@synthesize statusMessage;
@synthesize queuePosition;

@synthesize URL;

@synthesize artist;
@synthesize album;
@synthesize title;
@synthesize genre;
@synthesize year;
@synthesize track;

@synthesize totalFrames;
@synthesize bitrate;
@synthesize channels;
@synthesize bitsPerSample;
@synthesize sampleRate;

@synthesize seekable;

+ (void)initialize { 
	[self setKeys:[NSArray arrayWithObjects:@"artist",@"title",nil] triggerChangeNotificationsForDependentKey:@"display"]; 
	[self setKeys:[NSArray arrayWithObjects:@"totalFrames",nil] triggerChangeNotificationsForDependentKey:@"length"]; 
	[self setKeys:[NSArray arrayWithObjects:@"URL",nil] triggerChangeNotificationsForDependentKey:@"path"]; 
	[self setKeys:[NSArray arrayWithObjects:@"URL",nil] triggerChangeNotificationsForDependentKey:@"filename"]; 
}

- (void)setProperties:(NSDictionary *)dict
{
	[self setTotalFrames:	[[dict objectForKey:@"totalFrames"	] longLongValue]];
	[self setBitrate:		[[dict objectForKey:@"bitrate"		] intValue]];
	[self setChannels:		[[dict objectForKey:@"channels"		] intValue]];
	[self setBitsPerSample:	[[dict objectForKey:@"bitsPerSample" ] intValue]];
	[self setSampleRate:	[[dict objectForKey:@"sampleRate"	] floatValue]];
	[self setSeekable:		[[dict objectForKey:@"seekable"		] boolValue]];
}

- (void)readPropertiesThread
{
	NSDictionary *properties = [AudioPropertiesReader propertiesForURL:self.URL];
	if (!properties) {
		self.status = kCogEntryError;
		self.statusMessage = @"Failed to read properties!";

		return;
	}
	
	[self performSelectorOnMainThread:@selector(setProperties:) withObject:properties waitUntilDone:YES];
}

- (void)setMetadata: (NSDictionary *)m
{
	NSString *ti = [m objectForKey:@"title"];

	if (ti == nil || [ti isEqualToString:@""]) {
		[self setTitle:[[self.URL path] lastPathComponent]];
	}
	else {
		[self setTitle:ti];
	}
	
	[self setArtist:[m objectForKey:@"artist"	]];
	[self setAlbum:	[m objectForKey:@"album"	]];
	[self setGenre:	[m objectForKey:@"genre"	]];
	[self setYear:	[m objectForKey:@"year"		]];
	[self setTrack:	[[m objectForKey:@"track"	] intValue]];
}	

- (void)readMetadataThread
{
	NSDictionary *metadata = [AudioMetadataReader metadataForURL:self.URL];

	[self performSelectorOnMainThread:@selector(setMetadata:) withObject:metadata waitUntilDone:YES];

}

- (NSString *)description
{
	return [NSString stringWithFormat:@"PlaylistEntry %i:(%@)", self.index, self.URL];
}

- (NSString *)display
{
	if ((self.artist == NULL) || ([self.artist isEqualToString:@""]))
		return self.title;
	else {
		return [NSString stringWithFormat:@"%@ - %@", self.artist, self.title];
	}
}

- (double)length
{
	return ((double)self.totalFrames / self.sampleRate);
}

- (NSString *)path
{
	return [[self.URL path] stringByAbbreviatingWithTildeInPath];
}

- (NSString *)filename
{
	return [[self.URL path] lastPathComponent];
}

@end
