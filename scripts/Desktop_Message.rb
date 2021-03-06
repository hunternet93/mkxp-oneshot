# Displays desktop message boxes
class Desktop_Message
  HORIZ_MARGIN = 8
  VERT_MARGIN = 8
  HEIGHT = 160

  #--------------------------------------------------------------------------
  # * Object Initialization
  #--------------------------------------------------------------------------
  def initialize
    @viewport = Viewport.new(0, 0, 640, 480)
    @sprite_bg = Sprite.new(@viewport)
    @sprite_bg.bitmap = RPG::Cache.picture('cg_desktop_messagebox') #Bitmap.new(640, 480)
    @sprite_bg.zoom_x = @sprite_bg.zoom_y = 2
    @sprite_text = Sprite.new(@viewport)
    @contents = Bitmap.new(640, HEIGHT)
    @sprite_text.bitmap = @contents
    @sprite_text.y = (480 - HEIGHT) / 2
    @sprite_bg.z = 0
    @sprite_text.z = 1
    @sprite_text.zoom_x = @sprite_text.zoom_y = 1
    @viewport.z = 9999
    @viewport.visible = false

    @sprite_bg.opacity = 0
    @sprite_text.opacity = 0

    # Animation flags
    @fade_in = false
    @fade_out = false
  end
  #--------------------------------------------------------------------------
  # * Dispose
  #--------------------------------------------------------------------------
  def dispose
    terminate_message
    $game_temp.message_window_showing = false
    @contents.dispose
    #@sprite_bg.dispose
    @sprite_text.dispose
    @viewport.dispose
  end
  #--------------------------------------------------------------------------
  # * Terminate Message
  #--------------------------------------------------------------------------
  def terminate_message
    # Call message callback
    if !@skip_message_proc && $game_temp.message_proc != nil
      $game_temp.message_proc.call
      $game_temp.message_proc = nil
    end
    $game_temp.message_desktop_text = nil
  end
  #--------------------------------------------------------------------------
  # * Refresh: Load new message text and pre-process it
  #--------------------------------------------------------------------------
  def refresh
    # Initialize
    text = ''
    y = -1
    widths = []

    # Pre-process text
    text_raw = $game_temp.message_desktop_text.to_str

    # Substitute variables, actors, player name, newlines, etc
    text_raw.gsub!(/\\v\[([0-9]+)\]/) do
      $game_variables[$1.to_i]
    end
    text_raw.gsub!(/\\n\[([0-9]+)\]/) do
      $game_actors[$1.to_i] != nil ? $game_actors[$1.to_i].name : ""
    end
    text_raw.gsub!("\\p", $game_oneshot.player_name)
    text_raw.gsub!("\\n", "\n")
    # Handle text-rendering escape sequences
    text_raw.gsub!(/\\c\[([0-9]+)\]/, "\000[\\1]")
    # Finally convert the backslash back
    text_raw.gsub!("\\\\", "\\")

    # Now split text into lines by measuring text metrics
    x = y = 0
    maxwidth = @contents.width - HORIZ_MARGIN * 2
    spacesize = @contents.text_size(' ')
    for i in text_raw.split(/ /)
      # Split each word around newlines
      newline = false
      for j in i.split("\n")
        # Handle newline
        if newline
          text << "\n"
          widths << x
          x = 0
          y += 1
        else
          newline = true
        end

        # Get width of this word and see if it goes out of bounds
        width = @contents.text_size(j.gsub(/\000\[[0-9]+\]/, '')).width
        if x + width > maxwidth
          text << "\n"
          x = 0
          y += 1
        end

        # Append word to list
        if x == 0
          text << j
        else
          text << ' ' << j
        end
        x += width + spacesize.width
      end
    end
    widths << x if y < 4

    # Prepare renderer
    @contents.clear
    @contents.font.color = Color.new(44, 37, 54, 255)
    y_top = (HEIGHT - widths.length * 24) / 2
    x = (640 - widths[0]) / 2
    y = 0

    # Get 1 text character in c (loop until unable to get text)
    while ((c = text.slice!(/./m)) != nil)
      # \n
      if c == "\n"
        y += 1
        x = (640 - widths[y]) / 2
        next
      end
      # \c[n]
      if c == "\000"
        # Change text color
        text.sub!(/\[([0-9]+)\]/, "")
        color = $1.to_i
        if color >= 0 and color <= 7
          @contents.font.color = Window_Base.text_color(color)
        end
        # go to next text
        next
      end
      # Draw text
      @contents.draw_text(x+2, y_top + y * 24, 40, 24, c)
      # Add x to drawn text width
      x += @contents.text_size(c).width
    end
    Graphics.frame_reset
  end
  #--------------------------------------------------------------------------
  # * Frame Update
  #--------------------------------------------------------------------------
  def update
    # Handle fade-out effect
    if @fade_out
      @sprite_text.opacity -= 200
      @sprite_bg.opacity -= 200
      if @sprite_text.opacity <= 0
        @sprite_bg.opacity = 0
        @fade_out = false
        @viewport.visible = false
        $game_temp.message_window_showing = false
      end
      return
    end

    # Handle fade-in effect
    if @fade_in
      @sprite_text.opacity += 200
      @sprite_bg.opacity += 200
      if @sprite_text.opacity >= 255
      @sprite_bg.opacity = 255
        @fade_in = false
        $game_temp.message_window_showing = true
      end
      return
    end

    if visible
      if Input.trigger?(Input::ACTION) || Input.trigger?(Input::CANCEL)
        terminate_message
        @fade_out = true
      end
    end

    # Message is over and should be hidden or advanced to next
    if $game_temp.message_desktop_text == nil
      @fade_out = true if @viewport.visible
    else
      if !@viewport.visible
        # Fade in
        refresh
        @viewport.visible = true
        @fade_in = true
        Audio.se_play("Audio/SE/pc_messagebox.wav", 90, 150)
      end
    end

  end
  #--------------------------------------------------------------------------
  # * Variables
  #--------------------------------------------------------------------------
  def visible
    @viewport.visible
  end
  def visible=(val)
    @viewport.visible = val
  end
end
