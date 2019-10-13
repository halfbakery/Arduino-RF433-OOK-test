#define RF_RECEIVER_PIN		2
#define	MAX_RING_SIZE		2048
#define	MAX_FRAMES		16

#define	MIN_PULSE_LENGTH	10
#define	MIN_PULSE_NUM		16
#define	MAX_PULSE_NUM		256
#define	MIN_GAP_LENGTH		2000
#define	MAX_GAP_LENGTH		10000

struct frame
{
    int		head;
    int		size;
};

static volatile unsigned long	pulse_start;
static volatile bool		pulse_sync = false;
static volatile uint16_t	ring_data[MAX_RING_SIZE];
static volatile int		ring_tail = 0;
static volatile int		ring_size = MAX_RING_SIZE;
static volatile struct frame	received_frames[MAX_FRAMES];
static volatile int		received_num = 0;
static volatile struct frame*	current_frame = NULL;



inline void  receive_restart(bool sync)
{
    ring_tail		= current_frame->head;
    current_frame->size	= 0;
    pulse_sync		= sync;
}



inline void receive_finished(bool sync)
{
    pulse_sync		= sync;
    ring_size		-= current_frame->size;
    received_num++;

    if (received_num < MAX_FRAMES && ring_size >= MIN_PULSE_NUM * 2)
    {
	current_frame		= received_frames + received_num;
	current_frame->head	= ring_tail;
	current_frame->size	= 0;
    }
    else
    {
	current_frame		= NULL;
    }
}



static void receive_interrupt_handler(void)
{
    unsigned long	now = micros();
    unsigned long	pulse_length = now - pulse_start;
    bool		pulse_level = digitalRead(RF_RECEIVER_PIN);

    pulse_start	= now;
    if (!current_frame) return;

    if (pulse_length < MIN_PULSE_LENGTH)
    {
	if (current_frame->size >= MIN_PULSE_NUM * 2)
	{
	    receive_finished(false);
	}
	else
	{
	    receive_restart(false);
	}
    }
    else
    {
	if (!pulse_sync)
	{
	    if (pulse_length >= MIN_GAP_LENGTH && pulse_level)
	    {
		pulse_sync	= true;
	    }
	}
	else
	{
	    ring_data[ring_tail]= pulse_length;
	    ring_tail		= (ring_tail < MAX_RING_SIZE - 1) ? ring_tail + 1 : 0;
	    current_frame->size++;
	    if (pulse_length >= MAX_GAP_LENGTH && pulse_level)
	    {
		if (current_frame->size >= MIN_PULSE_NUM * 2)
		{
		    receive_finished(true);
		}
		else
		{
		    receive_restart(true);
		}
	    }
	    else if (current_frame->size >= MAX_PULSE_NUM * 2)
	    {
		receive_finished(false);
	    }
	    else if (current_frame->size >= ring_size)
	    {
		receive_finished(false);
	    }
	}
    }
}



void setup(void)
{
    pulse_start			= micros();
    received_frames[0].head	= 0;
    received_frames[0].size	= 0;
    current_frame		= received_frames;

    Serial.begin(115200);
    pinMode(RF_RECEIVER_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(RF_RECEIVER_PIN), receive_interrupt_handler, CHANGE);
}



static void show_pulses(int n, int head)
{
  Serial.println(";pulse data");
  Serial.println(";version 1");
  Serial.println(";timescale 1us");
  
  n	/= 2;
  Serial.print(";ook ");
  Serial.print(n);
  Serial.println(" pulses");
  
  Serial.println(";freq1 433849088");
  
  for (; n > 0; n--)
  {
    Serial.print(ring_data[head]);
    head= (head < MAX_RING_SIZE - 1) ? head + 1 : 0;
    Serial.print(' ');
    Serial.println(ring_data[head]);
    head= (head < MAX_RING_SIZE - 1) ? head + 1 : 0;
  }

  Serial.println(";end");
}



void loop()
{
    if (received_num > 0)
    {
	show_pulses(received_frames[0].size, received_frames[0].head);
	noInterrupts();
	ring_size	+= received_frames[0].size;
	memcpy(received_frames, received_frames + 1, sizeof(received_frames) - sizeof(received_frames[0]));
	received_num--;
	if (!current_frame)
	{
	    current_frame	= received_frames + received_num;
	    current_frame->head	= ring_tail;
	    current_frame->size	= 0;
	    pulse_sync		= false;
	}
	else
	{
	    current_frame--;
	}
	interrupts();
    }
}
