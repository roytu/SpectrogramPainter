#include <SFML/Graphics.hpp>
#include <vector>
#include <stdio.h>
#include <sndfile.h>
#include <complex>
#include <iostream>
#include <fftw3.h>
#include <sstream>

const int WIDTH = 400;
const int HEIGHT = 400;

const int SAMPLE_COUNT = 400;
const int SAMPLE_RATE = 400;
const int MAX_FREQ = 400; //in Hz
const int WINDOW_SIZE = MAX_FREQ;

const double MAX_AMPLITUDE = (1.0 * 0x7F000000);

sf::RenderWindow* App = new sf::RenderWindow(sf::VideoMode(WIDTH, HEIGHT), "SpectrogramPainter");
const sf::Input &Input = App->GetInput();

struct Point
{
    int x;
    int y;
    Point(int x, int y)
    {
        this->x = x;
        this->y = y;
    }
};

std::vector<std::vector<double> > gridCosine;
std::vector<std::vector<double> > gridSine;
std::vector<Point> changedPoints;

enum GridType {COSINE, SINE};
GridType gridType;

double yToHertz(int y)
{
    return (1 - ((double)y / HEIGHT)) * MAX_FREQ;
}

int hertzToY(int hertz)
{
    return (1 - ((double)hertz / MAX_FREQ)) * HEIGHT;
}

void compileWav()
{
    std::cerr << "Compiling...";

    SF_INFO* info = new SF_INFO();
    info->frames = SAMPLE_COUNT;
    info->samplerate = SAMPLE_RATE;
    info->channels = 1;
    info->format = (SF_FORMAT_WAV | SF_FORMAT_PCM_24);

    fftw_complex *in, *out;
    fftw_plan p;

    int N = WINDOW_SIZE;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

    p = fftw_plan_dft_1d(N, in, out, FFTW_BACKWARD, FFTW_ESTIMATE);

    std::vector<double> output = std::vector<double>(SAMPLE_COUNT, 0);

    for(int w = 0; w < SAMPLE_COUNT; w++)
    {
        std::vector<std::complex<double> > ifftList = std::vector<std::complex<double> >(WINDOW_SIZE, 0);

        for(int hertz = 1; hertz < MAX_FREQ; hertz++)
        {
            int y = hertzToY(hertz);

            double a = gridCosine.at(w).at(y);
            double b = gridSine.at(w).at(y);
            std::complex<double> freqC = std::complex<double>(a / 2, b / 2);

            ifftList.at(hertz) += -freqC;
            ifftList.at(N-hertz) += freqC;
        }

        //std::cout << "Input: " << w << std::endl;
        for(int i=0; i<N; i++)
        {
            in[i][0] = ifftList.at(i).real();
            in[i][1] = ifftList.at(i).imag();
            //std::cout << in[i][0] << " " << in[i][1] << std::endl;
        }

        fftw_execute(p);

        //std::cout << std::endl << "OUTPUT: " << w << std::endl;
        for(int i=0; i<N; i++)
        {
            int o = w - WINDOW_SIZE / 2 + i;
            //double wi = 0.5 * (1 - cos(2 * M_PI * (double)(i)) / N) * WINDOW_SIZE;
            //double wi = 1;
            double wi = WINDOW_SIZE;
            if(o >= 0 && o < SAMPLE_COUNT)
            {
                output.at(o) += out[i][0] / wi;
                //std::cout << o << ": " << out[i][0] << std::endl;
            }
            else
            {
                //std::cout << o << ": " << 0 << std::endl;
            }
        }
    }
    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(out);

    //std::cout << "TOTAL OUTPUT" << std::endl;
    for(int i=0; i<output.size(); i++)
    {
        //std::cout << output.at(i) << std::endl;
    }

    SNDFILE* file = sf_open("test.wav", SFM_WRITE, info);
    double *buffer;
    buffer = new double[N];
    for(int i=0; i<N; i++)
    {
        buffer[i] = (output.at(i) + 1) / 2 * MAX_AMPLITUDE;
    }
    sf_write_double(file, buffer, N);
    delete buffer;
    sf_close(file);
    delete info;
    output.clear();

    std::cerr << "DONE";
}

void init()
{
    std::cerr << "Initializing...";
    gridType = COSINE;
    gridCosine = std::vector<std::vector<double> >();
    for(int x = 0; x < WIDTH; x++)
    {
        std::vector<double> temp = std::vector<double>();
        for(int y = 0; y < HEIGHT; y++)
        {
            temp.push_back(0);
        }
        gridCosine.push_back(temp);
    }

    gridSine = std::vector<std::vector<double> >();
    for(int x = 0; x < WIDTH; x++)
    {
        std::vector<double> temp = std::vector<double>();
        for(int y = 0; y < HEIGHT; y++)
        {
            temp.push_back(0);
        }
        gridSine.push_back(temp);
    }

    changedPoints = std::vector<Point>();

    sf::Shape rect = sf::Shape::Rectangle(0, 0, WIDTH, HEIGHT, sf::Color::Black);
    App->Draw(rect);
}

void update()
{
    std::vector<std::vector<double> >* grid;
    switch(gridType)
    {
        case COSINE:
            grid = &gridCosine;
        break;
        case SINE:
            grid = &gridSine;
        break;
    }

    if(Input.IsMouseButtonDown(sf::Mouse::Left))
    {
        int x = Input.GetMouseX();
        int y = Input.GetMouseY();
        if(x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
        {
            grid->at(x).at(y) += 1;
            if(grid->at(x).at(y) > 1)
            {
                grid->at(x).at(y) = 1;
            }
            changedPoints.push_back(Point(x, y));
        }
    }
    if(Input.IsMouseButtonDown(sf::Mouse::Right))
    {
        int y = Input.GetMouseY();
        for(int x = 0; x < WIDTH; x++)
        {
            if(x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
            {
                grid->at(x).at(y) += 1;
                if(grid->at(x).at(y) > 1)
                {
                    grid->at(x).at(y) = 1;
                }
                changedPoints.push_back(Point(x, y));
            }
        }
    }
}

void draw()
{
    std::vector<std::vector<double> >* grid;
    switch(gridType)
    {
        case COSINE:
            grid = &gridCosine;
        break;
        case SINE:
            grid = &gridSine;
        break;
    }
    for(int i = 0; i < changedPoints.size(); i++)
    {
        Point point = changedPoints.at(i);
        double value = grid->at(point.x).at(point.y);
        sf::Shape rect = sf::Shape::Rectangle(point.x, point.y, point.x + 1, point.y + 1, sf::Color(value * 255, value * 255, value * 255));
        App->Draw(rect);
    }
    changedPoints.clear();

    sf::Color guiColor;
    switch(gridType)
    {
        case COSINE:
            guiColor = sf::Color::Red;
        break;
        case SINE:
            guiColor = sf::Color::Green;
        break;
    }

    sf::String Text;
    std::stringstream ss;
    ss << MAX_FREQ;
    Text = sf::String(ss.str() + " Hz", sf::Font::GetDefaultFont(), 12);
    Text.SetColor(guiColor);
    Text.Move(0, 0);
    App->Draw(Text);

    Text = sf::String("0 Hz", sf::Font::GetDefaultFont(), 12);
    Text.SetColor(guiColor);
    Text.Move(0, HEIGHT-20);
    App->Draw(Text);
}

int main()
{
    init();

    while (App->IsOpened())
    {
        sf::Event Event;
        while (App->GetEvent(Event))
        {
            if (Event.Type == sf::Event::Closed)
                App->Close();
            if (Event.Type == sf::Event::KeyPressed)
            {
                switch(Event.Key.Code)
                {
                    case sf::Key::Space:
                        switch(gridType)
                        {
                            case COSINE:
                                gridType = SINE;
                            break;
                            case SINE:
                                gridType = COSINE;
                            break;
                        }

                        App->Clear();
                        for(int x = 0; x < WIDTH; x++)
                        {
                            for(int y = 0; y < HEIGHT; y++)
                            {
                                changedPoints.push_back(Point(x, y));
                            }
                        }
                    break;
                    case sf::Key::Return:
                        compileWav();
                    break;
                    default:
                    break;
                }
            }
        }
        update();
        draw();
        App->Display();
    }

    return EXIT_SUCCESS;
}
