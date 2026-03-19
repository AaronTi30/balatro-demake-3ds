import os
from PIL import Image, ImageDraw

os.makedirs('assets/textures', exist_ok=True)

def draw_card_base():
    card_w, card_h = 56, 78
    img = Image.new('RGBA', (card_w, card_h), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    
    # Base: Whiter but still slightly off-white for paper texture feel
    bg_color = (250, 248, 245)
    border_color = (60, 60, 60)
    
    # Draw rounded rectangle
    d.rounded_rectangle([0, 0, card_w-1, card_h-1], radius=3, fill=bg_color, outline=border_color, width=2)
    
    # Subtle inner shading/imperfections
    d.line([(2, card_h-3), (card_w-3, card_h-3)], fill=(225, 220, 215), width=1)
    d.line([(card_w-3, 3), (card_w-3, card_h-3)], fill=(225, 220, 215), width=1)
    
    img.save('assets/textures/card_base.png')

def draw_suit_spade():
    size = 20
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    color = (30, 30, 30)
    x, y, w, h = 0, 0, size, size
    d.polygon([(x+w/2, y), (x+w, y+h*0.6), (x+w/2, y+h*0.8), (x, y+h*0.6)], fill=color)
    d.ellipse([x, y+h*0.4, x+w/2, y+h*0.8], fill=color)
    d.ellipse([x+w/2, y+h*0.4, x+w, y+h*0.8], fill=color)
    d.polygon([(x+w/2, y+h*0.6), (x+w*0.7, y+h), (x+w*0.3, y+h)], fill=color)
    img.save('assets/textures/spade.png')

def draw_suit_heart():
    size = 20
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    color = (220, 40, 40)
    x, y, w, h = 0, 0, size, size
    d.polygon([(x, y+h*0.3), (x+w, y+h*0.3), (x+w/2, y+h*0.9)], fill=color)
    d.ellipse([x, y, x+w/2+1, y+h*0.5], fill=color)
    d.ellipse([x+w/2-1, y, x+w, y+h*0.5], fill=color)
    img.save('assets/textures/heart.png')

def draw_suit_club():
    size = 20
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    color = (30, 30, 30)
    x, y, w, h = 0, 0, size, size
    d.ellipse([x+w/4, y, x+w*0.75, y+h*0.5], fill=color)
    d.ellipse([x, y+h*0.3, x+w*0.5, y+h*0.8], fill=color)
    d.ellipse([x+w*0.5, y+h*0.3, x+w, y+h*0.8], fill=color)
    d.polygon([(x+w/2, y+h*0.6), (x+w*0.7, y+h), (x+w*0.3, y+h)], fill=color)
    img.save('assets/textures/club.png')

def draw_suit_diamond():
    size = 20
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    color = (220, 60, 40)
    x, y, w, h = 0, 0, size, size
    d.polygon([(x+w/2, y), (x+w, y+h/2), (x+w/2, y+h), (x, y+h/2)], fill=color)
    img.save('assets/textures/diamond.png')

draw_card_base()
draw_suit_spade()
draw_suit_heart()
draw_suit_club()
draw_suit_diamond()
print("Generated assets!")
