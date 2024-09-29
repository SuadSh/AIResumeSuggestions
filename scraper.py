import sys # For communication between cpp and python
from bs4 import BeautifulSoup # BeautifulSoup for data exctraction
from selenium import webdriver # Webdriver to load the page
from selenium.webdriver.chrome.service import Service # Locate the webdriver via service
from selenium.webdriver.common.by import By # In order to find the username and password fiels

def jobScrape(url):
    # Set the Webdriver's path
    path = r"C:\\Users\\suads\\Desktop\\Projects\\chromedriver-win64\\chromedriver.exe"

    # Set up the selenium driver using Service
    service = Service(path)
    driver = webdriver.Chrome(service=service)

    # Go to the login page
    driver.get("https://www.linkedin.com/login")

    # Find username and password fields
    username = driver.find_element(By.ID, "username")
    password = driver.find_element(By.ID, "password")
    # Send my credentials to said fields
    username.send_keys("yourUsername")  
    password.send_keys("yourPassword")           

    # Submit the form
    driver.find_element(By.XPATH, "//button[contains(text(),'Sign in')]").click()

    try:
        # Load the given page
        driver.get(url)

        # Get the page and then parse it to BS
        page = driver.page_source
        soup = BeautifulSoup(page, 'html.parser')

        # Quit the driver since we got the data saved on soup 
        driver.quit()

        # Exctract data (Using strip=true so less tokens get passed to the AI)
        title = soup.select_one("h1.t-24.t-bold.inline").get_text(strip=True)
        description = soup.select_one("#job-details").get_text(strip=True)

        return title+" "+description
    except AttributeError as e:
        # Print error if occured
        print(f"Error: {e}")
    return None

# Run when called by the cpp script
if __name__ == "__main__":
    # Get the argument sent by the cpp program and pass it to the scraping function
    url = sys.argv[1]
    result = jobScrape(url)
    # If the result isn't empty print it
    if result:
        print(result) # Since they are connected through a pipe the cpp can capture the print
    else:
        # Print in case of failure to capture data
        print("Couldn't capture the job description")